/*
    SPDX-FileCopyrightText: 2009-2012 Stefan Majewsky <majewsky@gmx.net>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kgamethemeselector.h"
#include "kgamethemeselector_p.h"

// KF
#include <KLocalizedString>
// Qt
#include <QAbstractItemView>
#include <QApplication>
#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFont>
#include <QFontMetrics>
#include <QIcon>
#include <QListWidget>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QScrollBar>
#include <QVBoxLayout>

namespace Metrics
{
const int Padding = 6;
const QSize ThumbnailBaseSize(64, 64);
}

// BEGIN KGameThemeSelector

class KGameThemeSelectorPrivate
{
public:
    KGameThemeSelector *const q;

    KGameThemeProvider *m_provider;
    KGameThemeSelector::Options m_options;
    QListWidget *m_list;
    QString m_configFileName;

public:
    KGameThemeSelectorPrivate(KGameThemeProvider *provider, KGameThemeSelector::Options options, KGameThemeSelector *q)
        : q(q)
        , m_provider(provider)
        , m_options(options)
    {
    }

    void fillList();

    void _k_updateListSelection(const KGameTheme *theme);
    void _k_updateProviderSelection();
};

KGameThemeSelector::KGameThemeSelector(KGameThemeProvider *provider, Options options, QWidget *parent)
    : QWidget(parent)
    , d_ptr(new KGameThemeSelectorPrivate(provider, options, this))
{
    Q_D(KGameThemeSelector);

    d->m_list = new QListWidget(this);
    d->m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    d->m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    // load themes from provider
    d->fillList();

    // setup appearance of the theme list
    KGameThemeDelegate *delegate = new KGameThemeDelegate(d->m_list);
    QScreen *screen = QWidget::screen();
    QSize screenSize = screen->availableSize();
    if (screenSize.width() < 650 || screenSize.height() < 650) {
        d->m_list->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        if (parent) {
            d->m_list->setMinimumSize(0, 0);
        } else {
            // greater than zero to prevent zero sized dialog in some games
            d->m_list->setMinimumSize(330, 200);
        }
    } else {
        const QSize itemSizeHint = delegate->sizeHint(QStyleOptionViewItem(), QModelIndex());
        const QSize scrollBarSizeHint = d->m_list->verticalScrollBar()->sizeHint();
        d->m_list->setMinimumSize(itemSizeHint.width() + 2 * scrollBarSizeHint.width(), 4.1 * itemSizeHint.height());
    }

    // monitor change selection in both directions
    connect(d->m_provider, &KGameThemeProvider::currentThemeChanged, this, [this](const KGameTheme *theme) {
        Q_D(KGameThemeSelector);
        d->_k_updateListSelection(theme);
    });
    connect(d->m_list, &QListWidget::itemSelectionChanged, this, [this]() {
        Q_D(KGameThemeSelector);
        d->_k_updateProviderSelection();
    });
    // setup main layout
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(d->m_list);
    // KPat on Android: downloading new themes (KNewStuff) is not available
}

void KGameThemeSelector::setNewStuffConfigFileName(const QString &configName)
{
    Q_D(KGameThemeSelector);
    d->m_configFileName = configName;
}

KGameThemeSelector::~KGameThemeSelector() = default;

void KGameThemeSelectorPrivate::fillList()
{
    m_list->clear();
    const auto themes = m_provider->themes();
    for (const KGameTheme *theme : themes) {
        QListWidgetItem *item = new QListWidgetItem(theme->name(), m_list);
        item->setData(Qt::DecorationRole, m_provider->generatePreview(theme, Metrics::ThumbnailBaseSize));
        item->setData(KGameThemeDelegate::DescriptionRole, theme->description());
        item->setData(KGameThemeDelegate::AuthorRole, theme->author());
        item->setData(KGameThemeDelegate::AuthorEmailRole, theme->authorEmail());
        item->setData(KGameThemeDelegate::IdRole, theme->identifier());
    }
    _k_updateListSelection(m_provider->currentTheme());
}

void KGameThemeSelectorPrivate::_k_updateListSelection(const KGameTheme *theme)
{
    for (int idx = 0; idx < m_list->count(); ++idx) {
        QListWidgetItem *item = m_list->item(idx);
        const QByteArray thisId = item->data(KGameThemeDelegate::IdRole).toByteArray();
        if (thisId == theme->identifier()) {
            m_list->setCurrentItem(item, QItemSelectionModel::ClearAndSelect);
            return;
        }
    }
    // make sure that something is selected
    if (m_list->count() > 0) {
        m_list->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
    }
}

void KGameThemeSelectorPrivate::_k_updateProviderSelection()
{
    const QListWidgetItem *selItem = m_list->selectedItems().value(0);
    if (!selItem) {
        return;
    }
    const QByteArray selId = selItem->data(KGameThemeDelegate::IdRole).toByteArray();
    // select the theme with this identifier
    const auto themes = m_provider->themes();
    for (const KGameTheme *theme : themes) {
        if (theme->identifier() == selId) {
            m_provider->setCurrentTheme(theme);
        }
    }
}

class Q_DECL_HIDDEN KGameThemeSelector::Dialog : public QDialog
{
public:
    Dialog(KGameThemeSelector *sel, const QString &caption)
        : mSelector(sel)
    {
        QVBoxLayout *mainLayout = new QVBoxLayout;
        setLayout(mainLayout);
        mainLayout->addWidget(sel);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
        buttonBox->setStandardButtons(QDialogButtonBox::Close);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &KGameThemeSelector::Dialog::reject);
        // window caption
        if (caption.isEmpty()) {
            setWindowTitle(i18nc("@title:window config dialog", "Select theme"));
        } else {
            setWindowTitle(caption);
        }

        mainLayout->addWidget(buttonBox);
        show();
    }

protected:
    void closeEvent(QCloseEvent *event) override
    {
        event->accept();
        // delete myself, but *not* the KGameThemeSelector
        mSelector->setParent(nullptr);
        deleteLater();
    }

private:
    KGameThemeSelector *mSelector;
};

void KGameThemeSelector::showAsDialog(const QString &caption)
{
    if (!isVisible()) {
        new KGameThemeSelector::Dialog(this, caption);
    }
}

// END KGameThemeSelector
// BEGIN KGameThemeDelegate

KGameThemeDelegate::KGameThemeDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
    QAbstractItemView *view = qobject_cast<QAbstractItemView *>(parent);
    if (view)
        view->setItemDelegate(this);
}

QRect KGameThemeDelegate::thumbnailRect(QRect baseRect) const
{
    QRect thumbnailBaseRect(QPoint(Metrics::Padding + baseRect.left(), 0), Metrics::ThumbnailBaseSize);
    thumbnailBaseRect.moveCenter(QPoint(thumbnailBaseRect.center().x(), baseRect.center().y()));
    if (QApplication::isRightToLeft())
        thumbnailBaseRect.moveRight(baseRect.right() - Metrics::Padding);
    return thumbnailBaseRect;
}

void KGameThemeDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const bool rtl = option.direction == Qt::RightToLeft;
    QRect baseRect = option.rect;
    // draw background
    QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, nullptr);
    // draw thumbnail
    QRect thumbnailBaseRect = this->thumbnailRect(baseRect);
    const QPixmap thumbnail = index.data(Qt::DecorationRole).value<QPixmap>();
    QApplication::style()->drawItemPixmap(painter, thumbnailBaseRect, Qt::AlignCenter, thumbnail);

    // find metrics: text
    QStringList texts;
    QList<QFont> fonts;
    texts.reserve(3);
    fonts.reserve(3);
    {
        QString name = index.data(Qt::DisplayRole).toString();
        if (name.isEmpty())
            name = i18nc("@item name of theme, missing", "[No name]");
        texts << name;
        QFont theFont(painter->font());
        theFont.setBold(true);
        fonts << theFont;
    }
    {
        QString comment = index.data(DescriptionRole).toString();
        if (!comment.isEmpty()) {
            texts << comment;
            fonts << painter->font();
        }
    }
    {
        QString author = index.data(AuthorRole).toString();
        if (!author.isEmpty()) {
            const QString authorString = i18nc("@info author attribution", "By %1", author);
            texts << authorString;
            QFont theFont(painter->font());
            theFont.setItalic(true);
            fonts << theFont;
        }
    }
    // TODO: display AuthorEmailRole
    QList<QRect> textRects;
    textRects.reserve(texts.size());
    int totalTextHeight = 0;
    for (int i = 0; i < texts.count(); ++i) {
        QFontMetrics fm(fonts[i]);
        textRects << fm.boundingRect(texts[i]);
        textRects[i].setHeight(qMax(textRects[i].height(), fm.lineSpacing()));
        totalTextHeight += textRects[i].height();
    }
    QRect textBaseRect(baseRect);
    if (rtl) {
        textBaseRect.setRight(thumbnailBaseRect.left() - Metrics::Padding);
        textBaseRect.adjust(Metrics::Padding, Metrics::Padding, 0, -Metrics::Padding);
    } else {
        textBaseRect.setLeft(thumbnailBaseRect.right() + Metrics::Padding);
        textBaseRect.adjust(0, Metrics::Padding, -Metrics::Padding, -Metrics::Padding);
    }
    textBaseRect.setHeight(totalTextHeight);
    textBaseRect.moveTop(baseRect.top() + (baseRect.height() - textBaseRect.height()) / 2);
    // draw texts
    QRect currentTextRect(textBaseRect);
    painter->save();
    for (int i = 0; i < texts.count(); ++i) {
        painter->setFont(fonts[i]);
        const QRect &textRect = textRects[i];
        currentTextRect.setHeight(textRect.height());
        const QFontMetrics fm(fonts[i]);
        const QString text = fm.elidedText(texts[i], Qt::ElideRight, currentTextRect.width());
        painter->drawText(currentTextRect, Qt::AlignLeft | Qt::AlignVCenter, text);
        currentTextRect.moveTop(currentTextRect.bottom());
    }
    painter->restore();
}

QSize KGameThemeDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    // TODO: take text size into account
    return QSize(400, Metrics::ThumbnailBaseSize.height() + 2 * Metrics::Padding);
}

// END KGameThemeDelegate

#include "moc_kgamethemeselector.cpp"
