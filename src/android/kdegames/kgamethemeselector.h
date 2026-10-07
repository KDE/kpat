/*
    SPDX-FileCopyrightText: 2012 Stefan Majewsky <majewsky@gmx.net>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef KGAMETHEMESELECTOR_H
#define KGAMETHEMESELECTOR_H

// own
#include "kdegames_export.h"
#include "kgamethemeprovider.h"
// Qt
#include <QWidget>
// Std
#include <memory>

/**
 * @class KGameThemeSelector kgamethemeselector.h <KGameThemeSelector>
 * @brief Theme selection widget.
 *
 * This widget allows the user to change the theme selection of a
 * KGameThemeProvider. Selections are immediately applied to allow the user
 * to quickly preview themes. In simple cases, the widget can be used
 * standalone with the showAsDialog() method.
 *
 * @code
 * K_GLOBAL_STATIC_WITH_ARGS(KGameThemeSelector, selector, (provider))
 * ...
 * selector->showAsDialog();
 * @endcode
 */
class KDEGAMES_EXPORT KGameThemeSelector : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(KGameThemeSelector)

public:
    /// Flags which control the behavior of KGameThemeSelector.
    enum Option {
        DefaultBehavior = 0,
        /// Enable downloading of additional themes with KNewStuff3.
        /// This requires a KNS3 config file to be installed for this app.
        EnableNewStuffDownload = 1 << 0
    };
    /**
     * Stores a combination of #Option values.
     */
    Q_DECLARE_FLAGS(Options, Option)

    explicit KGameThemeSelector(KGameThemeProvider *provider, Options options = DefaultBehavior, QWidget *parent = nullptr);
    ~KGameThemeSelector() override;

    void setNewStuffConfigFileName(const QString &configFileName);

public Q_SLOTS:
    /// Create and show a non-modal dialog which displays this selector.
    /// The dialog will be automatically cleaned up when it's closed, but it
    /// is ensured that the selector is not deleted.
    ///
    /// This method does nothing if the selector widget is already visible.
    void showAsDialog(const QString &caption = QString());

private:
    class Dialog;
    std::unique_ptr<class KGameThemeSelectorPrivate> const d_ptr;
    Q_DECLARE_PRIVATE(KGameThemeSelector)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(KGameThemeSelector::Options)

#endif // KGAMETHEMESELECTOR_H
