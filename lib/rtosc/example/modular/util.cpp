const char *snip(const char *m)
{
    while(*m && *m!='/')++m;
    return *m?m+1:m;
}
