#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H

class NonCopyable
{
public:
    NonCopyable(const NonCopyable&) = delete; // deleted
    NonCopyable& operator = (const NonCopyable&) = delete; // deleted
    NonCopyable() = default;   // available
};

#endif // NONCOPYABLE_H
