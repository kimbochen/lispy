# Lispy Practice

This is the code I wrote when following through the amazing tutorial
[Build Your Own Lisp](https://buildyourownlisp.com/).  

## Compile Instructions

### Compile `mpc` to object file

```bash
# In mpc/
cc -std=c99 -Wall -c mpc.c
mv mpc.o ..
```

### Compile `main.c` to object file

```bash
cc -std=c99 -Wall -c main.c
```

### Link libraries and compile to binary

```bash
# -lm: Math library
# -ledit: editline library
cc -std=c99 -Wall main.o mpc.o -lm -ledit -o parsing
```
