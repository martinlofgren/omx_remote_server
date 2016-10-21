# AWK script to inline javascript and css files referenced to in a html file

BEGIN {
    js_regex = "<script[[:print:]]+src[[:space:]]*=[[:space:]]*\""
    css_regex = "<link[[:print:]]+rel[[:space:]]*=[[:space:]]*\"stylesheet\""
}

{
    if (match($0, js_regex)) {
	file = substr($0, RSTART+RLENGTH);
	sub("\".*</script>", "", file);
	print("<script>")
	while (i = getline < file ) {
	    print;
	}
	print("</script>");
    }

    else if (match($0, css_regex)) {
	file = $0;
	sub("^.*href[[:space:]]*=[[:space:]]*\"", "", file);
	sub("\".*>", "", file);
	print("<style type=\"text/css\">");
	while (i = getline < file ) {
	    print;
	}
	print("</style>");
    }

    else {
	print
    }
}


