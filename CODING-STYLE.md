# Summary

Every project has its coding style, and kmod is not an exception. This
document describes the preferred coding style for kmod code, in order to keep
some level of consistency among developers so that code can be easily
understood and maintained, and also to help your code survive under
maintainer's fastidious eyes so that you can get a passport for your patch
ASAP.

  * [Origin and inspiration](#inspiration)
    + [Line wrap](#line-wrap)
    + [Try to return/exit early](#try-to-return-exit-early)
    + [Avoid unnecessary initialization](#avoid-unnecessary-initialization)
    + [Sort the includes](#sort-the-includes)


# Inspiration

The kmod coding style is heavily based on the [Linux kernel]'s, oFono's and
BlueZ's. Below some basic rules:

## Line wrap

Wrap line at 90 char limit.

There are a few exceptions:

- Headers may or may not wrap
- If it's a string that is hitting the limit, it's preferred not to break in
order to be able to grep for that string. E.g:

    `err = my_function(ctx, "this is a long string that will pass the 80chr limit");`

- If code would become unreadable if line is wrapped
- If there's only one argument to the function, don't put it alone in a new line

Align the wrapped line with tab + spaces.

## Try to return/exit early

It's better to return/exit early in a function than having a really long
`if (...) { }`. Example:


    if (x) { // worse
        ...
        ...
        ...
        ...
    } else {
        return b;
    }
    return a;

    if (!x) // better
        return b;

    ...
    ...
    ...
    ...
    return a;

## Avoid unnecessary initialization

When declaring a variable, try not to initialize it unless necessary.

Example:

    int i = 1;  // wrong

    for (i = 0; i < 3; i++) {
    }

## Sort the includes

Let the includes in the following order, separated by a new line:

    < system headers >
    < shared/* >
    < libkmod >
    < tool >
    "local headers"

[Linux Kernel]: https://www.kernel.org/doc/html/v6.9/process/coding-style.html
