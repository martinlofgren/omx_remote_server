# AWK script to remove whitespace in html file

BEGIN {
    RS=""
}

length($0) > 1 {
    # Strip all excess whitespace    
    gsub("[ ]+", " ");
    gsub("[ ]+<", "<");
    gsub(">[ ]+", ">");
    gsub(";[ ]+", ";");
    gsub("[ ]+=", "=");
    gsub("=[ ]+", "=");
    
    # Print the line 
    print($0);
}
