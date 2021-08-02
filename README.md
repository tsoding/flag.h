# flag.h

Inspired by Go's flag module: https://pkg.go.dev/flag

**WARNING! The design of the library is not finished and may be a subject to change.**

## Quick Start

Check [example.c](./example.c)

```console
$ make
$ ./example-c -help
```

# Inlcuding globally

```
$ cd flag.h
$ sudo mkdir /usr/include/tsoding/
$ sudo mv flag.h /usr/include/tsoding
```
<details>
  <summary>Include any file</summary>
# Include any file
### usually the include file is located at
```
/usr/include
```
### if you need to include a file lets say the glad extension loader that nobody uses because glew exists
### the ``` /usr/include``` directory is owned by root so to move the glad folder from your ```HOME/Downloads/Glad/Glad``` to the include folder
### you would need to have root permissions, but today we have a tool called sudo that essentially goes heya um root gave him permissions 
### to do that. so we can write
```
$ sudo mv ~/Downloads/Glad/Glad
[sudo] password for $USER:
$
```
and bang you've just moved the Glad folder with glad.c/glad.h into the includes folder!
and now in your c file you can do 
#include<Glad/glad.h>
and it will work!
  </details>  
