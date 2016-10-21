# AWK script to inline javascript and css files referenced to in a html file

BEGIN {
    js_src_regex = "<script[[:print:]]+src[[:space:]]*=[[:space:]]*\""
}

{
    if (match($0, js_src_regex)) {
	file = substr($0, RSTART+RLENGTH);
	sub("\".*</script>", "", file);
	print("<script>")
	while (i = getline < file ) {
	    print;
	}
	print("</script>");
    }

    else {
	print
    }
}


