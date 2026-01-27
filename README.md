# Master of Files

🎓 **Trabajo Práctico – Sistemas Operativos**
🏫 *Universidad Tecnológica Nacional – Facultad Regional Buenos Aires (UTN-FRBA)*
📅 *Segundo cuatrimestre de 2025*

---

> 💾 Este trabajo práctico propone el desarrollo de un **sistema distribuido de gestión de archivos**, donde múltiples componentes cooperan para interpretar consultas, planificar su ejecución y persistir información de manera eficiente, aplicando conceptos centrales de la materia.

---

## 👥 Integrantes

* [Mateo Brancato](https://github.com/MateooSB)
* [German Carapezza](https://github.com/GerCarapezza)
* [Franco Ibañez](https://github.com/franmaxi)
* [Dalia Morel](https://github.com/daliamorel24)
* [Nicolás Scatolon](https://github.com/nicoScatolon)

---

## 📘 Links de Interés

[![PDF](https://img.shields.io/badge/Enunciado-PDF-red)](enunciados/Master%20of%20Files.pdf)
[![PDF](https://img.shields.io/badge/Pruebas-PDF-blue)](enunciados/2c2025%20-%20Master%20of%20Files.pdf)


---

## 🧠 Descripción general del proyecto

**Master of Files** es una simulación de una plataforma distribuida orientada al manejo de archivos y versiones. El sistema recibe *queries* compuestas por instrucciones de alto nivel, las planifica según distintos criterios y las ejecuta sobre un File System propio que soporta **paginación**, **caché**, **persistencia**, **deduplicación** y **concurrencia**.

Cada componente del sistema se ejecuta como un proceso independiente y se comunica con los demás mediante **sockets** y un **protocolo de mensajes**, permitiendo su despliegue en múltiples máquinas y el análisis del comportamiento bajo distintas cargas.

Los módulos que integran el sistema son:

* 🧩 **Query Control**: Franco Ibañez
* 🧠 **Master**: Franco Ibañez y Dalia Morel
* ⚙️ **Worker**: German Carapezza y Nicolás Scatolon
* 💾 **Storage**: Mateo Brancato y Franco Ibañez

---

## 🧩 Componentes del sistema

### 🧩 Query Control

El módulo **Query Control** es el punto de entrada al sistema. Su función es solicitar la ejecución de una *query* definida en un archivo, indicando además una prioridad asociada.

Responsabilidades principales:

* Conexión inicial con el módulo **Master**.
* Envío del path de la query junto con su prioridad.
* Recepción de mensajes de lectura generados durante la ejecución.
* Notificación del resultado final de la query.

Este módulo permite simular múltiples clientes solicitando operaciones concurrentes sobre el sistema.

---

### 🧠 Master

El **Master** actúa como el coordinador central del sistema, siendo responsable de administrar y planificar la ejecución de las queries.

Funciones clave:

* Recepción de queries desde múltiples Query Control.
* Asignación de identificadores únicos a cada query.
* Planificación de corto plazo mediante:

  * FIFO
  * Prioridades con desalojo
  * Mecanismo de *aging* para evitar inanición
* Coordinación dinámica con múltiples **Workers**.
* Reenvío de resultados parciales al Query Control correspondiente.
* Manejo de desconexiones de clientes y workers durante la ejecución.

Este módulo define el grado de multiprocesamiento del sistema según la cantidad de Workers conectados.

---

### ⚙️ Worker

El módulo **Worker** es el encargado de ejecutar efectivamente las queries, interpretando instrucción por instrucción.

Características principales:

* Interpretación secuencial de archivos de query.
* Ejecución de instrucciones como:
  `CREATE`, `TRUNCATE`, `READ`, `WRITE`, `TAG`, `COMMIT`, `FLUSH`, `DELETE` y `END`.
* Comunicación directa con **Storage** para operaciones persistentes.
* Implementación de una **memoria interna** con paginación bajo demanda.
* Soporte de algoritmos de reemplazo de páginas (LRU y CLOCK-M).
* Capacidad de ser desalojado y reanudar la ejecución desde un Program Counter específico.

Este módulo permite analizar el impacto del uso de memoria, los reemplazos de páginas y la latencia asociada.

---

### 💾 Storage

El módulo **Storage** implementa un File System propio sobre el sistema operativo anfitrión.

Responsabilidades:

* Inicialización y montaje del File System.
* Gestión de bloques físicos y lógicos.
* Administración de Files y Tags con control de versiones.
* Persistencia de datos mediante estructuras nativas y metadata.
* Implementación de **deduplicación de bloques** usando hashing MD5.
* Manejo de accesos concurrentes desde múltiples Workers.

Este componente representa el núcleo persistente del sistema y permite estudiar el funcionamiento interno de un sistema de archivos real.

---

✨ Este trabajo práctico integra conceptos de **planificación**, **memoria**, **sistemas distribuidos**, **file systems** y **sincronización**, consolidando los contenidos fundamentales de la materia en una implementación práctica.
