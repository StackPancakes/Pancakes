
// TODO: baremetal support later.
/*
struct Platform
{
    void (*putchar)(char);
    void (*raise)(const char*);
};

inline void default_putchar(char c)
{
    std::cout.put(c);
}

inline void default_raise(char const* msg)
{
    std::cerr << msg << '\n';
    std::exit(1);
}
*/