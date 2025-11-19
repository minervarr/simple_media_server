# Sistema de Depuración Autónoma para Claude

## El Problema

Claude no puede ver la GUI, pero necesita depurar aplicaciones gráficas. Cuando compila código que "funciona", puede haber:
- Botones mal alineados
- Funciones que no responden
- Crashes silenciosos
- Errores de renderizado

Sin poder VER la aplicación, Claude no puede saber si realmente funciona.

## La Solución: Logging Exhaustivo

Rediseñar el código para que sea **auto-observable** desde la terminal.

### Principios Clave

1. **CADA operación crítica debe loguear su resultado**
2. **Validar estado después de cada paso**
3. **Auto-terminar después de N frames para testing**
4. **Reportar estadísticas al final**

### Ejemplo de Implementación

```c
// ============================================
// LOGGING MACROS
// ============================================
#define LOG_INFO(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) do { \
    printf("[ERROR] " fmt "\n", ##__VA_ARGS__); \
    state.error_count++; \
} while(0)
#define LOG_SUCCESS(fmt, ...) printf("[✓] " fmt "\n", ##__VA_ARGS__)

// ============================================
// VALIDATION FUNCTIONS
// ============================================
static bool validate_resource(sg_buffer buf, const char* name) {
    sg_resource_state state = sg_query_buffer_state(buf);

    LOG_DEBUG("Validating %s:", name);
    LOG_DEBUG("  State: %s",
        state == SG_RESOURCESTATE_VALID ? "VALID" :
        state == SG_RESOURCESTATE_FAILED ? "FAILED" :
        state == SG_RESOURCESTATE_INVALID ? "INVALID" : "UNKNOWN");

    if (state != SG_RESOURCESTATE_VALID) {
        LOG_ERROR("%s validation FAILED!", name);
        return false;
    }

    LOG_SUCCESS("%s validation passed", name);
    return true;
}

// ============================================
// INITIALIZATION WITH LOGGING
// ============================================
static void init(void) {
    printf("\n═══════════════════════════════════════\n");
    printf("  INITIALIZATION - DEBUG MODE\n");
    printf("═══════════════════════════════════════\n\n");

    // Step 1: Initialize subsystem
    LOG_INFO("Step 1/5: Initializing graphics...");
    sg_setup(&(sg_desc){ /* ... */ });

    if (!validate_gfx_state("After sg_setup")) {
        LOG_ERROR("CRITICAL: Graphics init failed!");
        return;
    }
    LOG_SUCCESS("Graphics initialized");

    // Step 2: Create resources
    LOG_INFO("Step 2/5: Creating vertex buffer...");
    sg_buffer vb = sg_make_buffer(&(sg_buffer_desc){ /* ... */ });

    if (!validate_resource(vb, "Vertex buffer")) {
        return;
    }
    LOG_SUCCESS("Vertex buffer created");

    // ... más pasos ...

    printf("\n═══════════════════════════════════════\n");
    printf("  ✓ INIT COMPLETE - Errors: %d\n", state.error_count);
    printf("═══════════════════════════════════════\n\n");
}

// ============================================
// FRAME CALLBACK WITH PERIODIC LOGGING
// ============================================
static void frame(void) {
    state.frame_count++;

    // Log every 60 frames
    if (state.frame_count % 60 == 0) {
        LOG_DEBUG("Frame %d | FPS: ~%.1f", state.frame_count, calculate_fps());
    }

    // Validate every 60 frames
    if (state.frame_count % 60 == 0) {
        if (!validate_gfx_state("Frame validation")) {
            LOG_ERROR("Frame %d validation failed!", state.frame_count);
        }
    }

    // Auto-quit for testing
    if (state.frame_count >= TEST_DURATION_FRAMES) {
        LOG_INFO("Reached %d frames, auto-quitting", TEST_DURATION_FRAMES);
        sapp_request_quit();
    }

    // Render...
}

// ============================================
// INPUT WITH DETAILED LOGGING
// ============================================
static void input_event(const sapp_event* e) {
    switch (e->type) {
        case SAPP_EVENTTYPE_KEY_DOWN:
            LOG_INFO("Key pressed: code=%d", e->key_code);
            break;

        case SAPP_EVENTTYPE_MOUSE_DOWN:
            LOG_INFO("Mouse click at (%.1f, %.1f)", e->mouse_x, e->mouse_y);
            break;

        case SAPP_EVENTTYPE_RESIZED:
            LOG_INFO("Window resized to %dx%d", sapp_width(), sapp_height());
            break;
    }
}

// ============================================
// CLEANUP WITH STATISTICS
// ============================================
static void cleanup(void) {
    printf("\n═══════════════════════════════════════\n");
    printf("  CLEANUP & STATISTICS\n");
    printf("═══════════════════════════════════════\n");

    LOG_INFO("Total frames: %d", state.frame_count);
    LOG_INFO("Total errors: %d", state.error_count);
    LOG_INFO("Average FPS: %.1f", state.total_fps / state.frame_count);

    if (state.error_count > 0) {
        printf("  ⚠ COMPLETED WITH %d ERRORS\n", state.error_count);
    } else {
        printf("  ✓ COMPLETED SUCCESSFULLY\n");
    }

    printf("═══════════════════════════════════════\n\n");
}
```

## Flujo de Depuración Autónoma

```
┌─────────────────────────────────────┐
│ 1. Escribir código con logging     │
│    - LOG cada operación crítica     │
│    - Validar estado en cada paso    │
│    - Auto-quit después N frames     │
└───────────────┬─────────────────────┘
                │
┌───────────────▼─────────────────────┐
│ 2. Compilar con modo debug          │
│    - Capturar errores de compilador │
│    - Errores dicen QUÉ arreglar     │
└───────────────┬─────────────────────┘
                │
┌───────────────▼─────────────────────┐
│ 3. Ejecutar y capturar output       │
│    ./programa 2>&1 | tee output.log │
└───────────────┬─────────────────────┘
                │
┌───────────────▼─────────────────────┐
│ 4. Analizar output                  │
│    - ¿Todos los pasos completaron?  │
│    - ¿Hay errores reportados?       │
│    - ¿FPS estable?                  │
│    - ¿Eventos de input detectados?  │
└───────────────┬─────────────────────┘
                │
        ┌───────┴────────┐
        │                 │
     ¿OK?               ¿ERROR?
        │                 │
        ▼                 ▼
   ✓ DONE       ┌────────────────────┐
                │ 5. Corregir errores │
                │    - Leer logs      │
                │    - Identificar    │
                │    - Arreglar       │
                └────────┬────────────┘
                         │
                         └──────► Volver a (2)
```

## Casos de Uso

### Caso 1: Compilación Falla

**Output del compilador:**
```
error: 'SOKOL_GLCORE33' undeclared
fatal error: GL/gl.h: No such file or directory
```

**Acción:**
1. Cambiar `SOKOL_GLCORE33` a `SOKOL_GLCORE`
2. Instalar `libgl-dev`
3. Recompilar

### Caso 2: Compilación OK pero Runtime Falla

**Output del programa:**
```
[INFO] Step 1/5: Initializing graphics...
[ERROR] Sokol GFX validation FAILED at 'After sg_setup'
[ERROR] CRITICAL: Graphics init failed!
```

**Acción:**
1. Graphics init falló
2. Agregar más logging en `sg_setup`
3. Verificar que display esté disponible
4. Tal vez necesita `DISPLAY=:0` o modo headless

### Caso 3: Programa Corre pero Eventos No Funcionan

**Output del programa:**
```
[INFO] Frame 60 | FPS: ~60.0
[INFO] Frame 120 | FPS: ~60.0
[INFO] Frame 180 | FPS: ~60.0
```

**Problema:** No hay logs de `[INPUT]` - eventos no se detectan

**Acción:**
1. Verificar que `event_cb` esté configurado
2. Verificar que ventana tenga foco
3. Agregar más logging en `input_event()`

### Caso 4: Crash Silencioso

**Output del programa:**
```
[INFO] Step 1/5: Initializing graphics...
[✓] Graphics initialized
[INFO] Step 2/5: Creating vertex buffer...
```

**Problema:** Se detiene en Step 2

**Acción:**
1. Agregar logging DENTRO de creación de vertex buffer
2. Validar cada campo del descriptor
3. Verificar que datos sean válidos

## Checklist de Validación

Al ejecutar el programa, verificar:

- [ ] Todos los pasos de inicialización completan
- [ ] Todas las validaciones pasan (`[✓]`)
- [ ] No hay mensajes `[ERROR]`
- [ ] Frames se renderizan (log every N frames)
- [ ] FPS es razonable (~60 FPS)
- [ ] Eventos de input se detectan
- [ ] Programa termina limpiamente
- [ ] Estadísticas finales muestran 0 errores

## Comandos Útiles

```bash
# Compilar con debug symbols
cmake .. -DCMAKE_BUILD_TYPE=Debug
make VERBOSE=1

# Ejecutar y capturar todo
./programa 2>&1 | tee run.log

# Buscar errores en log
grep ERROR run.log
grep FAILED run.log

# Contar frames renderizados
grep "Frame.*FPS" run.log | wc -l

# Ver estadísticas finales
tail -20 run.log
```

## Errores Comunes y Soluciones

| Error | Causa | Solución |
|-------|-------|----------|
| `sg_isvalid() = FALSE` | Sokol init falló | Verificar contexto OpenGL/Display |
| `RESOURCESTATE_FAILED` | Recurso inválido | Verificar descriptor, tamaño, datos |
| `RESOURCESTATE_INVALID` | ID inválido | No usar recursos después de sg_destroy |
| Sin logs de Frame | Loop no ejecuta | Verificar que `frame_cb` esté set |
| Sin logs de Input | Eventos no llegan | Verificar `event_cb` y foco de ventana |
| FPS muy bajo (<10) | Rendering muy lento | V-sync? Validación costosa? |
| Crash sin logs | Segfault | Añadir más checkpoints de logging |

## Herramientas de Diagnóstico

### 1. strace - Calls del sistema

```bash
strace -e openat,read,write ./programa 2>&1 | grep -v "ENOENT"
```

Ver qué archivos intenta abrir, qué syscalls falla.

### 2. GDB - Debugging

```bash
gdb ./programa
(gdb) run
# Si crash:
(gdb) backtrace
(gdb) info locals
```

### 3. Valgrind - Memory errors

```bash
valgrind --leak-check=full ./programa
```

## Conclusión

Con logging exhaustivo, Claude puede:

1. ✅ **Compilar** y detectar errores de sintaxis/API
2. ✅ **Ejecutar** y capturar el output completo
3. ✅ **Analizar** el output para encontrar problemas
4. ✅ **Corregir** iterativamente hasta que funcione
5. ✅ **Validar** que todo funcionó correctamente

Todo sin necesitar VER la GUI - el programa **se auto-reporta** mediante logs.

Este es el sistema que permite depuración **completamente autónoma**.
