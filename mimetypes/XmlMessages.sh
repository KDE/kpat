function get_files
{
    echo kpatience.xml
}

function po_for_file
{
    case "$1" in
       kpatience.xml)
           echo kpat_mimetypes.po
       ;;
    esac
}

function tags_for_file
{
    case "$1" in
       kpatience.xml)
           echo comment
       ;;
    esac
}

