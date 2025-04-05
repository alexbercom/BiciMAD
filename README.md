# 🚲 Simulación BiciMAD 🧵
## 📌 Descripción
Este proyecto es una simulación multihilo del sistema de alquiler de bicicletas BiciMAD en la ciudad de Madrid. 
Cada usuario se representa como un hilo (`pthread`) que intenta coger y dejar bicicletas en diferentes estaciones de manera concurrente, respetando condiciones de sincronización y evitando interbloqueos.

Los usuarios siguen el siguiente flujo:
- Eligen aleatoriamente una estación para coger una bici.
- Esperan un tiempo aleatorio antes de intentarlo.
- Si no hay bicis disponibles, esperan o cambian de estación.
- Montan la bici durante un tiempo aleatorio.
- Eligen una estación aleatoria para dejarla.
- Si no hay huecos disponibles, esperan o cambian de estación.

La ejecución finaliza cuando todos los usuarios han completado sus paseos.

## 🛠️ Tecnologías utilizadas
- Lenguaje C
- Librerías POSIX Threads (`pthread.h`)
- Mutex y variables de condición
- Sincronización basada en `pthread_cond_wait` y `pthread_cond_timedwait`
- Entrada/salida en ficheros
- Gestión dinámica de memoria

## 🏗️ Estructura del Proyecto

### Archivos
- `BiciMAD.c`: Código principal de la práctica.
- `entrada_BiciMAD.txt`: Archivo con la configuración predeterminada del sistema (usuarios, estaciones, huecos...).
- `entrada.txt`: Archivo con la configuración del sistema no predeterminado pero elegible.
- `entrada_grande.txt`: Archivo con la configuración del sistema no predeterminado pero elegible el cual tiene datos de entrada más grandes.

### Funciones principales
- `main`: Inicializa los datos y lanza los hilos que simulan a los usuarios.
- `th_func`: Función asociada a cada hilo/usuario. Simula el proceso de coger y dejar bicis.
- `asignar_entradasalida`: Asigna los archivos de entrada y salida según los argumentos.
- `leer_entrada`: Lee y valida la configuración desde el archivo de entrada.
- `imprimir`: Imprime mensajes tanto en consola como en fichero.
- `obtener_fechayhora`: Genera una cadena de fecha y hora para nombrar el archivo de salida.
- `mostrar_config`: Imprime la configuración inicial cargada desde el archivo.

## ▶️ Ejecución

### 📦 Compilación
```bash
gcc BiciMAD.c -o BiciMAD -lpthread
```

### ▶️ Formas de ejecución
1. Sin argumentos (usa entrada_BiciMAD.txt por defecto):
```bash
./BiciMAD
```
2. Con archivo de entrada especificado (por ejemplo entrada.txt, que es una entrada más grande):
```bash
./BiciMAD entrada.txt
```
3. Con archivo de entrada y salida personalizados:
```bash
./BiciMAD entrada.txt salida.txt
```

## ⚙️ Formato del archivo de entrada
Debe tener 9 líneas, una por cada valor:
1. Usuarios totales
2. Número de estaciones
3. Huecos por estación
4. Tiempo mínimo para decidir tomar una bici
5. Tiempo máximo para decidir tomar una bici
6. Tiempo mínimo montando una bici
7. Tiempo máximo montando una bici
8. Número mínimo de paseos por usuario
9. Número máximo de paseos por usuario

## 🧪 Casos de prueba
- ✅ Simulación con configuraciones pequeñas y grandes (usuarios, estaciones).
- ✅ Evitar interbloqueos e inanición mediante señales y cambios de estación.
- ✅ Control de acceso concurrente a las estaciones con mutex y variables de condición.
- ✅ Gestión dinámica de bicis y huecos.
- ✅ Correcto uso de `pthread_cond_timedwait` para limitar tiempos de espera.
- ✅ Escritos sincronizados a consola y archivo de salida.

## 📈 Ejemplo de salida
```bash
SIMULACIÓN DE FUNCIONAMIENTO DE BiciMAD

Usuario 23 quiere coger bici de estacion 4
Usuario 23 coge bici de estacion 4
Usuario 23 montando bici...
Usuario 23 quiere dejar bici en estacion 9
Usuario 23 deja bici en estacion 9

SIMULACIÓN TERMINADA
```

## 👨‍💻 Autor
Alex Bermejo Compán
