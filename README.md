# Master of Files – Sistema Distribuido (UTN FRBA)

## Introduccion

Implementación y simulación de un **Sistema Distribuido modular**, desarrollado para la **Cátedra de Sistemas Operativos (UTN FRBA)**.
El objetivo principal es gestionar y persistir peticiones de manejo de **archivos (Files)** y **bloques** dentro de un entorno **concurrente**, aplicando conceptos de planificación, concurrencia, comunicación entre procesos y diseño de software de sistemas.

---

## Habilidades y Tecnologías Demostradas

### Área de Competencia | Detalles Clave de la Implementación

#### Sistemas Distribuidos y Concurrencia

* Diseño y desarrollo de una arquitectura de **cuatro módulos interconectados**: *Master*, *Worker*, *Query Control* y *Storage*.
* Capaces de ejecutarse en distintas máquinas o máquinas virtuales.
* Uso de mecanismos de **conectividad**, **serialización** y **sincronización** para garantizar consistencia y orden.

#### Planificación de Procesos (Scheduling)

* Implementación del **Módulo Master** como planificador de corto plazo de Queries.
* Soporte para dos algoritmos críticos:

  * **FIFO**
  * **Prioridades con desalojo (Preemptive Priority)**
* Implementación de **Aging** para evitar inanición (starvation) en Queries de baja prioridad.

#### Manejo de Memoria y Archivos

* Diseño del **Módulo Storage** orientado a la persistencia.
* Simulación de un sistema de archivos con:

  * *Superblock*
  * *Bitmap*
  * *Bloques físicos*
* Implementación de operaciones fundamentales:
  `CREATE`, `TRUNCATE`, `WRITE`, `READ`, `TAG`, `COMMIT`.
* Control de integridad y consistencia de datos.

#### Desarrollo de Software de Sistemas

Incluye:

* Uso de **archivos de configuración** para parametrizar módulos sin recompilar (puertos, algoritmo de planificación, aging, etc.).
* Implementación obligatoria de logs mínimos (nivel **LOG_LEVEL_INFO**).
* Definición de la arquitectura, estados de Queries (`READY`, `EXEC`, `EXIT`) y protocolos de comunicación.

---

## Estructura Modular y Responsabilidades

| Módulo            | Rol Principal                                                                                                                                                                      |
| ----------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Master**        | Control de flujo y planificación. Gestiona la cola de Queries, aplica el algoritmo configurado y coordina con Workers. Maneja conexión/desconexión dinámica de clientes y Workers. |
| **Worker**        | Unidad de ejecución. Interpreta Queries, gestiona operaciones sobre Files y administra su **Memoria Interna** con algoritmos de reemplazo de páginas.                              |
| **Storage**       | Persistencia física y lógica. Administra la asignación de bloques, metadatos y responde operaciones de lectura/escritura.                                                          |
| **Query Control** | Interfaz del usuario. Envía Queries con prioridad al Master y espera resultados o lecturas.                                                                                        |

