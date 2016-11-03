/*
 * Function: hash
 * --------------
 * Quick and simple hashing function used to speed up string lookups. The
 * function is taken from [http://www.cse.yorku.ca/~oz/hash.html] and
 * modified to create hashes of smaller value (the mod 4096 in the return 
 * statement).
 *
 * This probably isn't a good hash function for anything other purpose than
 * this, so you really do not want to use this!
 *
 * str: the string to hash
 * 
 * Returns: the hash for the given string
 */
unsigned int hash(const char *str) {
  unsigned int hash = 5381;
  int c;

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  
  return hash % 4096;
}

