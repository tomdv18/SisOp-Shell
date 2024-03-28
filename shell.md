# shell

### Búsqueda en $PATH

***¿Cuáles son las diferencias entre la syscall `execve(2)` y la familia de wrappers proporcionados por la librería estándar de C (libc) `exec(3)`?***

Las funciones exec son de mas alto nivel, permiten pasar argumentos separados por coma o como arreglo de char y utiliza el `PATH` para buscar el programa.

***¿Puede la llamada a exec(3) fallar? ¿Cómo se comporta la implementación de la shell en ese caso?***

Si puede fallar, devuelve un valor negativo y establece la variable _'errno'_. Esta implementacion de shell utiliza `execvp`, y se ejecuta `_exit(-1)` en caso de error.

---

### Procesos en segundo plano

***Detallar cuál es el mecanismo utilizado para implementar procesos en segundo plano***

Si el struct cmd tiene el tipo BACK, no se hace el `waitpid` en esa iteracion de `run_cmd`, en cambio, en todos las iteraciones de `run_cmd` se hace un `waitpid` especial con WNOHANG que limpiara al proceso en caso de haber terminado. Notese que en la entrega esta solucion ya no esta presente ya que se ha implementado el Segundo Plano avanzado.

***¿Por qué es necesario el uso de señales?***

La señal es necesaria ya que hace falta interrumpir al proceso padre apenas termina el proceso en el background, al contrario que la solucion anterior que checkeaba una vez por ciclo por procesos en el background finalizados.

---

### Flujo estándar

- ***Investigar el significado de 2>&1, explicar cómo funciona su forma general***
- ***Mostrar qué sucede con la salida de cat out.txt en el ejemplo.***
- ***Luego repetirlo, invirtiendo el orden de las redirecciones (es decir, 2>&1 >out.txt). ¿Cambió algo? Compararlo con el comportamiento en bash(1).***

Tomando la forma general de 2>&1:

    COMANDO >& ARCHIVO

Dónde `COMANDO` es aquello a ejecutar y `ARCHIVO` es a dónde querramos redirigir el flujo. 

Podemos encontrar dos formatos para redirección del standard output y standard error a un archivo (desde ahora ejemplificado con `ARCHIVO`).

     &>ARCHIVO
     >&ARCHIVO

Según la documentación de Bash, es preferible utilizar la primera ya que en la segunda forma `ARCHIVO` puede no expandirse a un número o a `-` (utilizado para cerrar el stdin y stdout), y de hacerlo podría aplicarse otros tipos de operadores de redirección generando problemas de compatibilidad. 
Esta segunda forma es, siempre que NO se trate con MULTIOS, semánticamente equivalente a: 

    >ARCHIVO 2>&1


Analizando el comando por partes tenemos que:
   - ***2>*** redirecciona stderr a ARCHIVO.
   - ***&1*** redirecciona stderr a stdout. 
   
En donde el "&" indica que lo que precede es un File Descriptor y no el nombre de un archivo

Usando la sintaxis de redirección I/O `2>&1` le informa a la shell que deseamos que stderr(file descriptor 2) se redirija al mismo lugar al que se este enviando el standard output (file descriptor 1).
Por lo que este comando enviaria (ya que la shell evalua las direcciones de I/O de izquierda a derecha) tanto el stdout como
el stderr a `ARCHIVO`. La shell logra la redirección del stderr duplicando el file descriptor 2 
para que referencie el mismo file abierto como file descriptor 1, efecto que puede lograrse con las syscalls `dup` y `dup2`.
Tal como se aclara en _The Linux Programming Interface_, no es suficiente conque la shell simplemente abra `ARCHIVO` dos veces, 
una en el file descriptor 1 y otra en el file descriptor 2, ya que no comparten un file offset pointer, y podrían terminar sobreescribiendo
el output del otro.

Como podemos al probar el ejemplo, se combinó la salida de standard error y output: 

    $ cat out.txt
    ls: cannot access '/noexiste': No such file or directory
    /home:
    g7

Al invertirlo, en nuestra shell seguimos teniendo el mismo resultado: 

    ls: cannot access '/noexiste': No such file or directory
    /home:
    g7

Al correrlo invertido en bash, obtenemos un resultado diferente, en el que solo vemos lo devuelto por el stderr: 

    $ ls -C /home /noexiste 2>&1  >out.txt
    ls: cannot access '/noexiste': No such file or directory

Según la documentación de Bash, solo se direcciona el standard output ya que el standard error se convierte en una copia del standard output antes
de que el standard output sea redireccionado a out.txt, siendo por lo tanto el orden de las redirecciones significante en Bash.

---

### Tuberías múltiples

***Investigar qué ocurre con el exit code reportado por la shell si se ejecuta un pipe. ¿Cambia en algo? ¿Qué ocurre si, en un pipe, alguno de los comandos falla? Mostrar evidencia (e.g. salidas de terminal) de este comportamiento usando bash. Comparar con su implementación.***

Si bien cada comando devuelve un exit code (también llamados exit status y report status), si estamos ejecutando una shell, ésta devolverá solamente el resultado del
último ejecutado. Si todos los comandos se ejecutaran con éxito, tendríamos un exit code 0. Pero si uno o más comandos fallan
el exit code será el del ultimo comando que falló, si eso detuviera la ejecución, teniendo en cuenta el orden secuencial de izquierda 
a derecha en el que se analizan los comandos. 

Ejemplo::
En primer lugar, veamos los resultados de los comandos separados.

    $ echo $0
    bash 
    $ echo $? 
    0
    $ hhhh
    hhhh: command not found 
    $ echo $? 
    127

Ahora, si utilizamos un pipe, vemos un resultado distinto cuando consultamos el exit code con `$?`

    $ echo $0 | hhhh
    hhhh: command not found
    $ echo $?
    127

De la misma manera, si negamos el outcome con el operador lógico NOT `!`, un exit code indicador de error (normalmente distinto de cero) de un pipe se transformará en un exit code 0. 
    
    $ ls | hhhh
    hhhh: command not found
    $ echo $?
    0

En cambio, si el exit code es distinto de cero pero esto no detiene la ejecución de ninguna manera, seguiremos obteniendo el exit code del último comando ejecutado. 

    $ false | true
    $ echo $?
    0                      
   
Si comparamos con nuestra shell, devolverá el último comando ejecutado. Si el último comando falla, este no influenciará 
el status code del pipe obtenido hasta el momento, o sea, el exit code del anteúltimo comando.
A su vez, tal como sucede en bash, los comandos vacíos e inexistentes serán ignorados 
y se seguirá ejecutando el pipe mientras sea posible. 

     $ echo "Hello" | grep "Goodbye" | wc -l
     0
     $ hhhh | grep "Hello" | wc -l
     0
     $ echo "Hello" | grep "Hello" | wc -l
     1
     $ echo "Hello" | grep "Hello" | hhhh
     $ echo $?
     0
    
    
    


---

### Variables de entorno temporarias

***¿Por qué es necesario hacerlo luego de la llamada a fork(2)?***

Cuando un nuevo proceso es creado a través de un `fork()`, hereda una copia del environment de su padre, incluyendo el conjunto de variables de environment que se mantienen dentro del user-space del proceso. 
Cuando un proceso reemplaza al programa con el uso de `exec()`, el nuevo programa hereda el environment usado o recibe uno nuevo especificamente como parte de la llamada a `exec()`. 
Para asegurarse la temporalidad de estas variables y que solo afecten al proceso hijo que queremos manipular, debemos asegurarnos de declararlas luego de realizado este `fork()` en el que herede el entorno de su padre, pero antes de la correspondiente ejecución con exec(). 




***En algunos de los wrappers de la familia de funciones de exec(3) (las que finalizan con la letra e), se les puede pasar un tercer argumento (o una lista de argumentos dependiendo del caso), con nuevas variables de entorno para la ejecución de ese proceso. Supongamos, entonces, que en vez de utilizar setenv(3) por cada una de las variables, se guardan en un arreglo y se lo coloca en el tercer argumento de una de las funciones de exec(3).***
- ***¿El comportamiento resultante es el mismo que en el primer caso? Explicar qué sucede y por qué.***
- ***Describir brevemente (sin implementar) una posible implementación para que el comportamiento sea el mismo.***

Una llamada a `execve()` (y su correspondiente a listas `execle()`) destruye los segmentos existentes (data, stack, heap, text), reemplazandolos. 
Teniendo en cuenta que el environment es una forma de traspasar cierta información de manera sencilla de un proceso hijo a uno padre, la utilización de estas funciones 
descartarian y reemplazarían la totalidad de las variables de entorno por las que esten en el array de argumentos. En cambio, al utilizar `setenv(3)`, estamos seteando variables,
ya sea existentes o inexistentes, en el proceso antes de realizar la ejecución, y estas se sumarán a las ya existentes, heredadas del proceso padre. A su vez, tenemos que tener en cuenta, que `setenv` solo afectará las variables ya existentes cuando se reciba un int en el parametro overwrite distinto de cero. Sino se mantendrían las heredadas. El proceso `exec` (en todas sus variantes no terminadas en e), tomaran el environment 
de la variable externa `environ` en la calling process. 

Para poder imitar el comportamiento de setenv con execve, tendriamos que resguardar todas las variables del proceso padre que sea necesario preservar en el array/lista. Si sabemos las key especificas, podemos directamente llamarlas con `getenv()`. 
A su vez, podríamos iterar `environ` para recuperar todas las variables de environment en `/usr/bin/env`. 

---

### Pseudo-variables

***Investigar al menos otras tres variables mágicas estándar, y describir su propósito.***

- $# -> Número de argumentos posicionales en base 10.

Ejemplo:: 
       
       #!usr/bin/env bash
       echo $#
       
       bash~$ ./script.sh e q cat $0
       4

- $1, $2, ..., $n -> Argumento en la linea de comando en esa posición (parámetro posicional). Si se llama a `./algun_script.sh x y z` con `echo $2`recibiriamos y como respuesta. En Bash, una vez que se supera el 9, los argumentos tienen que ponerse entre corchetes, por ejemplo ${12}

Ejemplo:: Tenemos un script simple que toma la información de una persona y devuelve el resultado, permitiendote printearlo en la configuraciòn que desees.

       #!usr/bin/env bash
       echo "Hi, my name is $1, my favorite color is $2, and I live in $3"
       
       bash~$ ./script_info.sh Mark red NYC
       Hi, my name is Mark, my favorite color is red, and I live in NYC 

- $@ -> Todos los argumentos como un array

Ejemplo::

       #!usr/bin/env bash
       i=0
       for param in "$@"
       do     
              let "i+=1"
              echo "Param #$i= $param"
       done
       
       bash~$ script.sh red blue 222 $0
       Param #1= red
       Param #2= blue
       Param #3= 222
       Param #4= bash

- $* -> Todos los argumentos como un solo string. 

Ejemplo::
 Si lo usamos sin comillas, los parámetros serán separados por espacios y expandiendo cualquier parametro que se le pase recursivamente (como será en este ejemplo $0). 

       #!usr/bin/env bash
       echo $*
       
       bash~$ ./script.sh 1 q cat $0
       1 q cat bash

Si lo usamos con comillas, serán separados por la variable IFS (International Field Separator). Por default IFS es un espacio (similar resultado a la variable sin comillas), pero puede ser modificado por el usuario. 

       #!usr/bin/env bash
       echo "$*"
       oldIFS="$IFS"
       IFS="-----"
       echo "$*"
       IFS=null
       echo "$*"
       IFS=oldIFS
       
       bash~$ ./script.sh 1 w cat $0
       1 w cat bash 
       1-w-cat-bash
       1nwncatnbash

- $0 -> Nombre del script siendo ejecutado

Ejemplo :: Veamos la diferencia entre invocarlo desde la command line en bash, donde se expande al nombre del shell actual.

       bash ~$ echo $0
       bash

También podemos invocarlo dentro de un Bash script, donde se expandirá al nombre del script. Si creamos un archivo llamado print_name.sh y lo corremos en el shell, veremos el nombre del script siendo echoed: 
 
      #!usr/bin/env bash
       echo "$0"
       
       bash ~$ ./print_name.sh 
       ./print_name.sh




_Fuente de los ejemplos :: medium.com_




---

### Comandos built-in

***¿Entre cd y pwd, alguno de los dos se podría implementar sin necesidad de ser built-in? ¿Por qué? ¿Si la respuesta es sí, cuál es el motivo, entonces, de hacerlo como built-in? (para esta última pregunta pensar en los built-in como true y false)***

`cd` debe ser built-in y no programa aparte, ya que se espera que se cambie el directorio en el que se
encuentra la shell, por lo cual debería efectuar el cambio sobre ese mismo proceso padre.

Podría implementarse `pwd` como programa aparte, dado que se posee la función getcwd(3) para obtener el path actual en el que se 
encuentre trabajando la shell. Sin embargo, la ventaja de implementar pwd como built-in es que nos ahorramos los llamados a las 
syscalls `fork()`, `exec()` y `wait()` que serian necesarios para ejecutar `pwd` como un proceso separado.
### Historial

---
