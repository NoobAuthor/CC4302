==========================================================
Esta es la documentación para compilar y ejecutar su tarea
==========================================================

Se está ejecutando el comando: less README.txt

***************************
*** Para salir: tecla q ***
***************************

Para avanzar a una nueva página: tecla <page down>
Para retroceder a la página anterior: tecla <page up>
Para avanzar una sola línea: tecla <enter>
Para buscar un texto: tecla / seguido del texto (/...texto...)
         por ejemplo: /ddd

-----------------------------------------------

Ud. debe crear el archivo sat.c y programar ahí la función buscar.
Ya hay una plantilla para sat.c en sat.c.plantilla.

Pruebe su tarea bajo Debian 12 de 64 bits.  Estos son los requerimientos
para aprobar su tarea:

+ make run debe felicitarlo por aprobar este modo de ejecución.
El speed up reportado debe ser de al menos 1.5.

+ make run-g debe felicitarlo.

+ make run-san debe felicitarlo y no reportar ningún error de manejo de memoria.

+ make run-thr debe felicitarlo y no reportar ningún datarace.

Cuando pruebe su tarea con make run en su notebook asegúrese de que
posea al menos 2 cores, que está configurado en modo alto rendimiento
y que no estén corriendo otros procesos intensivos en uso de CPU al
mismo tiempo.  De otro modo podría no lograr el speed up solicitado.

Invoque el comando make zip para ejecutar todos los tests y generar un
archivo sat.zip que contiene sat.c, con su solución, y resultados.txt,
con la salida de make run, make run-g, make run-san y make run-thr.

Para depurar use: make ddd
Tenga en consideración que depurar programas con procesos pesados es
más difícil que depurar programas secuenciales.

Video con ejemplos de uso de ddd: https://youtu.be/FtHZy7UkTT4
Archivos con los ejemplos: https://www.u-cursos.cl/ingenieria/2020/2/CC3301/1/novedades/r/demo-ddd.zip

-----------------------------------------------

Entrega de la tarea

Ejecute: make zip

Entregue por U-cursos el archivo sat.zip

A continuación es muy importante que descargue de U-cursos el mismo
archivo que subió.  Luego examine el archivo sat.c revisando
que es el correcto.  Es frecuente que no lo sea producto de un defecto
de U-cursos.

-----------------------------------------------

Limpieza de archivos

make clean

Hace limpieza borrando todos los archivos que se pueden volver
a reconstruir a partir de los fuentes: *.o binarios etc.

-----------------------------------------------

Acerca del comando make

El comando make sirve para automatizar el proceso de compilación asegurando
recompilar el archivo binario ejecutable cuando cambió uno de los archivos
fuentes de los cuales depende.

A veces es útil usar make con la opción -n para que solo muestre
exactamente qué comandos va a ejecutar, sin ejecutarlos de verdad.
Por ejemplo:

   make -n run-g

También es útil usar make con la opción -B para forzar la recompilación
de los fuentes a pesar de que no han cambiado desde la última compilación.
Por ejemplo:

   make -B run-san
