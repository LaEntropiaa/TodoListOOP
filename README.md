# TodoList
Es un pequeño programa para tener una lista de tareas. Esta hecha en c++ y compilado/manejado por Xmake para mayor facilidad y compatibilidad. El diseño esta pensado para aprovechar el paradigma orientado a objetos de c++, haciendo un uso modular y organizado que nos permitira desarrollar no solo la actual version de este programa, sino tambien permitir añadir funcionalidades a futuro (uso de sqlite3, libreria de cli, etc.).

## ¿Como usar?
Primero hay que descargar Xmake, se puede hacer aqui https://xmake.io/ . Tras descargarlo solo hace falta correr "xmake run" en el directorio principal y xmake deberia poder compilar y ejecutar el programa de manera correcta sin importar el sistema operativo.

## Funcionalidades
Permite operaciones CRUD basicas para tareas asi como mostrarlas en pantalla. Realmente es un diseño basico pero eso es intencional, buscamos ampliar sus capacidades --como persistencia con una base de datos local-- y visualizacion --uso de libreria dedicada para CLI.

## Diseño
Se compone principalmente de 3 clases que siguen una jerarquia clara:
- Tarea: Contiene la funcionalidad basica para manejar y almacenar tareas en memoria
- ListaTarea: Un wrapper sobre Vec para manejar tareas de manera facil, igual esta pensada mas como una interfaz para las demas tipos de lista de tareas que implementaremos a futuro
- UI: Mantiene acceso a la InboxLista --Version mas basica de ListaTarea-- para conocer el estado de cada tarea y extrae su informacion para mostrarla asi como se encarga de manejar input para cumplir su funcionalidad

Con esta estructura se mantienen las responsabilidades bien separadas siguiendo un diseño OOP, tambien para el equipo resultara mas facil añadir funcionalidades a futuro.
