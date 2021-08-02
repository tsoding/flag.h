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
  <h1> Include any file </h1>
  <h3> usually the include folder is located at </h3>
<pre>
/usr/include
</pre>
<h3> if you need to include a file lets say the glad extension loader that nobody uses because glew exists </h3>
  <h3> the  <pre> /usr/include</pre> directory is owned by root so to move the glad folder from your <pre> HOME/Downloads/Glad/Glad </pre> to the include folder </h3>
<h3> you would need to have root permissions, but today we have a tool called sudo that essentially goes heya um root gave him permissions </h3>
<h3> to do that. so we can write </h3>
<pre>
$ sudo mv ~/Downloads/Glad/Glad
[sudo] password for $USER:
$
</pre>
 <h3>
and bang you've just moved the Glad folder with glad.c/glad.h into the includes folder!
and now in your c file you can do 
<pre>
#include<   Glad/    glad.h  > 
</pre>
   Note: github thinks Glad is a html thing you would just type glad/glad.h or smth
   and it will work!
   </h3>
  </details>  
