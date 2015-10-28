<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.0" language="es_AR">
<context>
    <name>AboutDialog</name>
    <message>
        <source>About LMMS</source>
        <translation>Sobre LMMS</translation>
    </message>
    <message>
        <source>LMMS (Linux MultiMedia Studio)</source>
        <translation>LMMS (Linux MultiMedia Studio)</translation>
    </message>
    <message>
        <source>Version %1 (%2/%3, Qt %4, %5)</source>
        <translation>Versión %1 (%2/%3, Qt %4, %5)</translation>
    </message>
    <message>
        <source>About</source>
        <translation>Sobre</translation>
    </message>
    <message>
        <source>LMMS - easy music production for everyone</source>
        <translation>LMMS - producción musical fácil para tod@s</translation>
    </message>
    <message>
        <source>Authors</source>
        <translation>Autores</translation>
    </message>
    <message>
        <source>Translation</source>
        <translation>Traducción</translation>
    </message>
    <message>
        <source>Current language not translated (or native English).

If you&apos;re interested in translating LMMS in another language or want to improve existing translations, you&apos;re welcome to help us! Simply contact the maintainer!</source>
        <translation>La aplicación no ha sido traducida a tu idioma.

Si estás interesado en traducir LMMS a otro idioma o querés mejorar una traducción existente, eres bienvenido. ¡Simplemente contacta al equipo de desarrollo!
</translation>
    </message>
    <message>
        <source>License</source>
        <translation>Licencia</translation>
    </message>
    <message>
        <source>Copyright (c) 2004-2014, LMMS developers</source>
        <translation>Copyright (c) 2004-2014, LMMS developers</translation>
    </message>
    <message>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;a href=&quot;http://lmms.sourceforge.net&quot;&gt;&lt;span style=&quot; text-decoration: underline; color:#0000ff;&quot;&gt;http://lmms.sourceforge.net&lt;/span&gt;&lt;/a&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;a href=&quot;http://lmms.sourceforge.net&quot;&gt;&lt;span style=&quot; text-decoration: underline; color:#0000ff;&quot;&gt;http://lmms.sourceforge.net&lt;/span&gt;&lt;/a&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;&lt;br/&gt;
Traducción español Argentina por Equipo &lt;a href=&quot;http://huayra.conectarigualdad.gov.ar&quot;&gt;Huayra gnu/linux&lt;/a&gt;</translation>
    </message>
</context>
<context>
    <name>AudioAlsa::setupWidget</name>
    <message>
        <source>DEVICE</source>
        <translation>DISPOSITIVO</translation>
    </message>
    <message>
        <source>CHANNELS</source>
        <translation>CANALES</translation>
    </message>
</context>
<context>
    <name>AudioFileProcessorView</name>
    <message>
        <source>Open other sample</source>
        <translation>Abrir otra Muestra</translation>
    </message>
    <message>
        <source>Click here, if you want to open another audio-file. A dialog will appear where you can select your file. Settings like looping-mode, start and end-points, amplify-value, and so on are not reset. So, it may not sound like the original sample.</source>
        <translation>Clic acá para abrir un nuevo archivo de audio. Un cuadro de diálogo aparecerá donde podrás seleccionar tu archivo. Configuraciones como &quot;looping mode&quot;, puntos de comienzo y final y valor de amplitud no serán reestablecidos, por ello puede que no suene como la muestra original.</translation>
    </message>
    <message>
        <source>Reverse sample</source>
        <translation>Muestra en reversa</translation>
    </message>
    <message>
        <source>If you enable this button, the whole sample is reversed. This is useful for cool effects, e.g. a reversed crash.</source>
        <translation>Si activa este botón la muestra será reproducida al revés. Esto es útil para buenos efectos, como por ejemplo, golpes del revés.</translation>
    </message>
    <message>
        <source>Loop sample at start- and end-point</source>
        <translation>Buclear muestra entre puntos inicio y final</translation>
    </message>
    <message>
        <source>Here you can set, whether looping-mode is enabled. If enabled, AudioFileProcessor loops between start and end-points of a sample until the whole note is played. This is useful for things like string and choir samples.</source>
        <translation>Aquí se puede establecer, si es que el modo bucle está activado. Si se activa, AudioFileProcessor buclea entre el inicio y puntos finales de una muestra hasta que se reproduzca toda la nota. Esto es útil para cosas como las muestras de cuerda y los coros.</translation>
    </message>
    <message>
        <source>Amplify:</source>
        <translation>Amplificar:</translation>
    </message>
    <message>
        <source>With this knob you can set the amplify ratio. When you set a value of 100% your sample isn&apos;t changed. Otherwise it will be amplified up or down (your actual sample-file isn&apos;t touched!)</source>
        <translation>Con este mando se puede establecer la relación de amplitud. Cuando se establece un valor de 100% su muestra no se cambia. De lo contrario, se amplificará arriba o abajo (su muestra-original no se modificará!)</translation>
    </message>
    <message>
        <source>Startpoint:</source>
        <translation>Punto inicial:</translation>
    </message>
    <message>
        <source>With this knob you can set the point where AudioFileProcessor should begin playing your sample. If you enable looping-mode, this is the point to which AudioFileProcessor returns if a note is longer than the sample between the start and end-points.</source>
        <translation>Con este mando se puede establecer el punto en el que AudioFileProcessor debe comenzar a reproducir su muestra.</translation>
    </message>
    <message>
        <source>Endpoint:</source>
        <translation>Punto final:</translation>
    </message>
    <message>
        <source>With this knob you can set the point where AudioFileProcessor should stop playing your sample. If you enable looping-mode, this is the point where AudioFileProcessor returns if a note is longer than the sample between the start and end-points.</source>
        <translation>Con este mando se puede establecer el punto en el que AudioFileProcessor debería dejar de reproducir su muestra.</translation>
    </message>
    <message>
        <source>Continue sample playback across notes</source>
        <translation>Continuar la reproducción de la muestra a través de notas</translation>
    </message>
    <message>
        <source>Enabling this option makes the sample continue playing across different notes - if you change pitch, or the note length stops before the end of the sample, then the next note played will continue where it left off. To reset the playback to the start of the sample, insert a note at the bottom of the keyboard (&lt; 20 Hz)</source>
        <translation>Habilitar esta opción hace que la muestra se siga reproduciendo a través de diferentes notas - si cambia el tono, o la duración de la nota se detiene antes de la final de la muestra, a continuación, la siguiente nota tocada continuará donde lo dejó. Para restablecer la reproducción para el inicio de la muestra, insertar una nota en la parte inferior del teclado (&lt;20 Hz)</translation>
    </message>
</context>
<context>
    <name>AudioFileProcessorWaveView</name>
    <message>
        <source>Sample length:</source>
        <translation>Duración de muestra:</translation>
    </message>
</context>
<context>
    <name>AudioJack</name>
    <message>
        <source>JACK client restarted</source>
        <translation>Cliente JACK reiniciado</translation>
    </message>
    <message>
        <source>LMMS was kicked by JACK for some reason. Therefore the JACK backend of LMMS has been restarted. You will have to make manual connections again.</source>
        <translation>LMMS fue rechazado por JACK por alguna razón. Por lo tanto el servidor JACK de LMMS se ha reiniciado. Deberá hacer las conexiones manuales de nuevo.</translation>
    </message>
    <message>
        <source>JACK server down</source>
        <translation>Servidor JACK caído</translation>
    </message>
    <message>
        <source>The JACK server seems to have been shutdown and starting a new instance failed. Therefore LMMS is unable to proceed. You should save your project and restart JACK and LMMS.</source>
        <translation>El servidor JACK parece haber sido bajado y una nueva instancia falló.al iniciar. Por lo tanto LMMS es incapaz de continuar. Debe guardar el proyecto y reiniciar JACK y LMMS.</translation>
    </message>
</context>
<context>
    <name>AudioJack::setupWidget</name>
    <message>
        <source>CLIENT-NAME</source>
        <translation>NOMBRE-CLIENTE</translation>
    </message>
    <message>
        <source>CHANNELS</source>
        <translation>CANALES</translation>
    </message>
</context>
<context>
    <name>AudioOss::setupWidget</name>
    <message>
        <source>DEVICE</source>
        <translation>DISPOSITIVO</translation>
    </message>
    <message>
        <source>CHANNELS</source>
        <translation>CANALES</translation>
    </message>
</context>
<context>
    <name>AudioPortAudio::setupWidget</name>
    <message>
        <source>BACKEND</source>
        <translation>SEGUNDO-PLANO</translation>
    </message>
    <message>
        <source>DEVICE</source>
        <translation>DISPOSITIVO</translation>
    </message>
</context>
<context>
    <name>AudioPulseAudio::setupWidget</name>
    <message>
        <source>DEVICE</source>
        <translation>DISPOSITIVO</translation>
    </message>
    <message>
        <source>CHANNELS</source>
        <translation>CANALES</translation>
    </message>
</context>
<context>
    <name>AudioSdl::setupWidget</name>
    <message>
        <source>DEVICE</source>
        <translation>DISPOSITIVO</translation>
    </message>
</context>
<context>
    <name>AutomatableModel</name>
    <message>
        <source>&amp;Reset (%1%2)</source>
        <translation>&amp;Restaurar (%1%2)</translation>
    </message>
    <message>
        <source>&amp;Copy value (%1%2)</source>
        <translation>&amp;Copiar valor (%1%2)</translation>
    </message>
    <message>
        <source>&amp;Paste value (%1%2)</source>
        <translation>&amp;Pegar valor (%1%2)</translation>
    </message>
    <message>
        <source>Edit song-global automation</source>
        <translation>Editar automatización global de la canción</translation>
    </message>
    <message>
        <source>Connected to %1</source>
        <translation>Conectado a %1</translation>
    </message>
    <message>
        <source>Connected to controller</source>
        <translation>Conectado a controlador</translation>
    </message>
    <message>
        <source>Edit connection...</source>
        <translation>Editar conexión...</translation>
    </message>
    <message>
        <source>Remove connection</source>
        <translation>Eliminar conexión</translation>
    </message>
    <message>
        <source>Connect to controller...</source>
        <translation>Conectar a controlador...</translation>
    </message>
    <message>
        <source>Remove song-global automation</source>
        <translation>Quitar automatización global de la canción</translation>
    </message>
    <message>
        <source>Remove all linked controls</source>
        <translation>Quitar todos los controles vinculados</translation>
    </message>
</context>
<context>
    <name>AutomationEditor</name>
    <message>
        <source>Play/pause current pattern (Space)</source>
        <translation>Reproducir/Pausar el patrón actual (Espaciador)</translation>
    </message>
    <message>
        <source>Stop playing of current pattern (Space)</source>
        <translation>Detener la reproducción del patrón actual (Espaciador)</translation>
    </message>
    <message>
        <source>Click here if you want to play the current pattern. This is useful while editing it.  The pattern is automatically looped when the end is reached.</source>
        <translation>Haga clic aquí si desea reproducir el patrón actual. Esto es útil durante la edición de la misma. El patrón se repite automáticamente cuando se llega al final.</translation>
    </message>
    <message>
        <source>Click here if you want to stop playing of the current pattern.</source>
        <translation>Haga clic aquí si desea detener la reproducción del patrón actual.</translation>
    </message>
    <message>
        <source>Draw mode (Shift+D)</source>
        <translation>Modo Dibujar (Shift+D)</translation>
    </message>
    <message>
        <source>Erase mode (Shift+E)</source>
        <translation>Modo Borrar (Shift+E)</translation>
    </message>
    <message>
        <source>Click here and draw-mode will be activated. In this mode you can add and move single values.  This is the default mode which is used most of the time.  You can also press &apos;Shift+D&apos; on your keyboard to activate this mode.</source>
        <translation>Clic acá para activar el modo dibujo. En este modo se pueden agregar y mover los valores individuales. Éste es el modo por defecto que se utiliza la mayor parte del tiempo. También puede pulsar &apos;Shift + D&apos; en el teclado para activar este modo.</translation>
    </message>
    <message>
        <source>Click here and erase-mode will be activated. In this mode you can erase single values. You can also press &apos;Shift+E&apos; on your keyboard to activate this mode.</source>
        <translation>Clic acá para activar el modo Borrar. En este modo se pueden eliminar valores individuales. También puede pulsar &apos;Shift + E&apos; en el teclado para activar este modo.</translation>
    </message>
    <message>
        <source>Cut selected values (Ctrl+X)</source>
        <translation>Cortar valores seleccionados (Ctrl+X)</translation>
    </message>
    <message>
        <source>Copy selected values (Ctrl+C)</source>
        <translation>Copiar valores seleccionador (Ctrl+C)</translation>
    </message>
    <message>
        <source>Paste values from clipboard (Ctrl+V)</source>
        <translation>Pegar valores desde el portapapeles (Ctrl+V)</translation>
    </message>
    <message>
        <source>Click here and selected values will be cut into the clipboard.  You can paste them anywhere in any pattern by clicking on the paste button.</source>
        <translation>Clic acá para Cortar los valores seleccionados. Puedes pegarlos donde quieras en un patrón haciendo clic en el botón pegar.</translation>
    </message>
    <message>
        <source>Click here and selected values will be copied into the clipboard.  You can paste them anywhere in any pattern by clicking on the paste button.</source>
        <translation>Clic acá para copiar los valores seleccionados al portapapeles. Puedes pegarlos donde quieras en un patrón haciendo clic en el botón pegar.</translation>
    </message>
    <message>
        <source>Click here and the values from the clipboard will be pasted at the first visible measure.</source>
        <translation>Clic acá para pegar los valores del portapapeles en la primera medida visible.</translation>
    </message>
    <message>
        <source>Automation Editor - no pattern</source>
        <translation>Editor de Automatización - sin patrón</translation>
    </message>
    <message>
        <source>Automation Editor - %1</source>
        <translation>Editor de Automatización -%1 </translation>
    </message>
    <message>
        <source>Please open an automation pattern with the context menu of a control!</source>
        <translation>
Por favor, abra un patrón de automatización con el menú contextual de un control!</translation>
    </message>
    <message>
        <source>Values copied</source>
        <translation>Valores copiados</translation>
    </message>
    <message>
        <source>All selected values were copied to the clipboard.</source>
        <translation>Todos los valores seleccionados fueron copiados al portapapeles.</translation>
    </message>
    <message>
        <source>Discrete progression</source>
        <translation>Progresión Discreta</translation>
    </message>
    <message>
        <source>Linear progression</source>
        <translation>Progresión lineal</translation>
    </message>
    <message>
        <source>Cubic Hermite progression</source>
        <translation>
Progresión &quot;Hermite Cubic&quot;</translation>
    </message>
    <message>
        <source>Tension: </source>
        <translation>Tensión: </translation>
    </message>
    <message>
        <source>Click here to choose discrete progressions for this automation pattern.  The value of the connected object will remain constant between control points and be set immediately to the new value when each control point is reached.</source>
        <translation>Haga clic aquí para elegir progresiones discretas para este patrón de automatización. El valor del objeto conectado permanecerá constante entre los puntos de control y será inmediatamente definido al nuevo valor cuando se alcance cada punto de control.</translation>
    </message>
    <message>
        <source>Click here to choose linear progressions for this automation pattern.  The value of the connected object will change at a steady rate over time between control points to reach the correct value at each control point without a sudden change.</source>
        <translation>Haga clic aquí para elegir progresiones lineales para este patrón de automatización. El valor del objeto conectado va a cambiar a un ritmo constante en el tiempo entre los puntos de control para alcanzar el valor correcto en cada punto de control, sin un cambio repentino.</translation>
    </message>
    <message>
        <source>Click here to choose cubic hermite progressions for this automation pattern.  The value of the connected object will change in a smooth curve and ease in to the peaks and valleys.</source>
        <translation>Haga clic aquí para elegir progresiones Hermite cubic de este patrón de automatización. El valor del objeto conectado cambiará en una curva suave y facilidad en picos y valles.</translation>
    </message>
    <message>
        <source>Tension value for spline</source>
        <translation>Valor de tensión para SPline</translation>
    </message>
    <message>
        <source>A higher tension value may make a smoother curve but overshoot some values.  A low tension value will cause the slope of the curve to level off at each control point.</source>
        <translation>Un valor de tensión más alta puede hacer una curva más suave pero sobrepasar algunos valores. Un valor bajo de tensión hará que la pendiente se estabilice en cada punto de control.</translation>
    </message>
</context>
<context>
    <name>AutomationPattern</name>
    <message>
        <source>Drag a control while pressing &lt;Ctrl&gt;</source>
        <translation>Para arrastrar un control presione &lt;Ctrl&gt; y luego arrástrelo</translation>
    </message>
</context>
<context>
    <name>AutomationPatternView</name>
    <message>
        <source>double-click to open this pattern in automation editor</source>
        <translation>haga doble clic para abrir este patrón en el editor de automatización</translation>
    </message>
    <message>
        <source>Open in Automation editor</source>
        <translation>Abrir en Editor de Automatización</translation>
    </message>
    <message>
        <source>Clear</source>
        <translation>Limpiar</translation>
    </message>
    <message>
        <source>Reset name</source>
        <translation>Restablecer nombre</translation>
    </message>
    <message>
        <source>Change name</source>
        <translation>Cambiar nombre</translation>
    </message>
    <message>
        <source>%1 Connections</source>
        <translation>%1 Conexiones</translation>
    </message>
    <message>
        <source>Disconnect &quot;%1&quot;</source>
        <translation>Desconectar &quot;%1&quot;</translation>
    </message>
</context>
<context>
    <name>AutomationTrack</name>
    <message>
        <source>Automation track</source>
        <translation>Pista de Automatización</translation>
    </message>
</context>
<context>
    <name>Controller</name>
    <message>
        <source>Controller %1</source>
        <translation>Controlador %1</translation>
    </message>
</context>
<context>
    <name>ControllerConnectionDialog</name>
    <message>
        <source>Connection Settings</source>
        <translation>Configuración de Conexión</translation>
    </message>
    <message>
        <source>MIDI CONTROLLER</source>
        <translation>Controlador MIDI</translation>
    </message>
    <message>
        <source>Input channel</source>
        <translation>Canal de entrada</translation>
    </message>
    <message>
        <source>CHANNEL</source>
        <translation>CANALES</translation>
    </message>
    <message>
        <source>Input controller</source>
        <translation>Controlador de entrada</translation>
    </message>
    <message>
        <source>CONTROLLER</source>
        <translation>CONTROLADOR</translation>
    </message>
    <message>
        <source>Auto Detect</source>
        <translation>Auto Detectar</translation>
    </message>
    <message>
        <source>MIDI-devices to receive MIDI-events from</source>
        <translation>Dispositivos MIDI desde los cuales recibir eventos MIDI</translation>
    </message>
    <message>
        <source>USER CONTROLLER</source>
        <translation>CONTROLADOR del USUARIO</translation>
    </message>
    <message>
        <source>MAPPING FUNCTION</source>
        <translation>FUNCIÓN DE MAPEO</translation>
    </message>
    <message>
        <source>OK</source>
        <translation>OK</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation>Cancelar</translation>
    </message>
    <message>
        <source>LMMS</source>
        <translation>LMMS</translation>
    </message>
    <message>
        <source>Cycle Detected.</source>
        <translation>Ciclo detectado.</translation>
    </message>
</context>
<context>
    <name>ControllerRackView</name>
    <message>
        <source>Controller Rack</source>
        <translation>Bastidor de Controladores</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Agregar</translation>
    </message>
    <message>
        <source>Confirm Delete</source>
        <translation>Confirmar Eliminar</translation>
    </message>
    <message>
        <source>Confirm delete? There are existing connection(s) associted with this controller. There is no way to undo.</source>
        <translation>Confirmar Eliminar? existe al menos una conexión asociada con este controlador. No hay forma de deshacer la acción.</translation>
    </message>
</context>
<context>
    <name>ControllerView</name>
    <message>
        <source>Controls</source>
        <translation>Controles</translation>
    </message>
    <message>
        <source>Controllers are able to automate the value of a knob, slider, and other controls.</source>
        <translation>
Los controladores son capaces de automatizar el valor de un deslizador y otros controles.</translation>
    </message>
    <message>
        <source>Rename controller</source>
        <translation>Renombrar controlador</translation>
    </message>
    <message>
        <source>Enter the new name for this controller</source>
        <translation>Ingrese un nuevo nombre para este controlador</translation>
    </message>
    <message>
        <source>&amp;Remove this plugin</source>
        <translation>&amp;Remover este complemento</translation>
    </message>
    <message>
        <source>&amp;Help</source>
        <translation>&amp;Ayuda</translation>
    </message>
</context>
<context>
    <name>Effect</name>
    <message>
        <source>Effect enabled</source>
        <translation>Efecto Activado</translation>
    </message>
    <message>
        <source>Wet/Dry mix</source>
        <translation>Wet/Dry mix</translation>
    </message>
    <message>
        <source>Gate</source>
        <translation>Gate</translation>
    </message>
    <message>
        <source>Decay</source>
        <translation>Decay</translation>
    </message>
</context>
<context>
    <name>EffectChain</name>
    <message>
        <source>Effects enabled</source>
        <translation>Efectos activados</translation>
    </message>
</context>
<context>
    <name>EffectRackView</name>
    <message>
        <source>EFFECTS CHAIN</source>
        <translation>Cadena de efectos</translation>
    </message>
    <message>
        <source>Add effect</source>
        <translation>Agregar efecto</translation>
    </message>
</context>
<context>
    <name>EffectSelectDialog</name>
    <message>
        <source>Add effect</source>
        <translation>Agregar efecto</translation>
    </message>
    <message>
        <source>Plugin description</source>
        <translation>Descripción de Complemento</translation>
    </message>
</context>
<context>
    <name>EffectView</name>
    <message>
        <source>Toggles the effect on or off.</source>
        <translation>Alterna el efecto entre encendido o apagado.</translation>
    </message>
    <message>
        <source>On/Off</source>
        <translation>Encendido/Apagado</translation>
    </message>
    <message>
        <source>W/D</source>
        <translation>W/D (entrada/salida)</translation>
    </message>
    <message>
        <source>Wet Level:</source>
        <translation>Nivel de Wet (entrada):</translation>
    </message>
    <message>
        <source>The Wet/Dry knob sets the ratio between the input signal and the effect signal that forms the output.</source>
        <translation>La perilla de Wet / Dry establece la relación entre la señal de entrada y la señal de efecto que forma la salida.</translation>
    </message>
    <message>
        <source>DECAY</source>
        <translation>DECAY</translation>
    </message>
    <message>
        <source>Time:</source>
        <translation>Tiempo:</translation>
    </message>
    <message>
        <source>The Decay knob controls how many buffers of silence must pass before the plugin stops processing.  Smaller values will reduce the CPU overhead but run the risk of clipping the tail on delay and reverb effects.</source>
        <translation>La perillal DECAY controla el número de búferes de silencio deben pasar antes de que el complemento detenga el procesamiento. Los valores más bajos reducen la sobrecarga de la CPU, pero corren el riesgo de saturación de la cola de retardo y reverb.</translation>
    </message>
    <message>
        <source>GATE</source>
        <translation>GATE</translation>
    </message>
    <message>
        <source>Gate:</source>
        <translation>Gate:</translation>
    </message>
    <message>
        <source>The Gate knob controls the signal level that is considered to be &apos;silence&apos; while deciding when to stop processing signals.</source>
        <translation>La perilla de puerta (Gate) controla el nivel de señal que es considerado como &quot;silencio&quot; al decidir cuándo dejar de procesar señales.</translation>
    </message>
    <message>
        <source>Controls</source>
        <translation>Controles</translation>
    </message>
    <message>
        <source>Effect plugins function as a chained series of effects where the signal will be processed from top to bottom.

The On/Off switch allows you to bypass a given plugin at any point in time.

The Wet/Dry knob controls the balance between the input signal and the effected signal that is the resulting output from the effect.  The input for the stage is the output from the previous stage. So, the &apos;dry&apos; signal for effects lower in the chain contains all of the previous effects.

The Decay knob controls how long the signal will continue to be processed after the notes have been released.  The effect will stop processing signals when the volume has dropped below a given threshold for a given length of time.  This knob sets the &apos;given length of time&apos;.  Longer times will require more CPU, so this number should be set low for most effects.  It needs to be bumped up for effects that produce lengthy periods of silence, e.g. delays.

The Gate knob controls the &apos;given threshold&apos; for the effect&apos;s auto shutdown.  The clock for the &apos;given length of time&apos; will begin as soon as the processed signal level drops below the level specified with this knob.

The Controls button opens a dialog for editing the effect&apos;s parameters.

Right clicking will bring up a context menu where you can change the order in which the effects are processed or delete an effect altogether.</source>
        <translation>Los Complementos de Efectos funcionan como una serie encadenada de efectos que transforman la señal de arriba a abajo.
El interruptor de encendido / apagado le permite eludir un plugin en concreto en un momento en el tiempo.

La perilla de Wet / Dry controla el balance entre la señal de entrada y la señal con efecto que es la salida resultante del efecto. La entrada para la etapa es la salida de la etapa anterior. Así, la señal &quot;seca&quot; para los efectos más bajos en la cadena contiene todos los efectos anteriores.

Los controles de Decay definen cuánto tiempo seguirá la señal siendo procesada ​​después de que las notas han sido liberadas. El efecto se detendrá cuando el volumen ha caído por debajo de un umbral dado durante un período de tiempo dado. Este mando establece el &quot;tiempo determinado&quot;. Tiempos más largos requieren más CPU, por lo que este número debe fijarse bajo para la mayoría de los efectos. Necesita ser aumentado para los efectos que producen largos períodos de silencio, por ejemplo, retrasos.

La perilla de puerta (gate) controla el &apos;umbral dado &quot;para el apagado automático del efecto. El reloj para el &apos;tiempo determinado&apos; comenzará tan pronto como el nivel de la señal procesada caiga por debajo del nivel especificado con este mando.

El botón Controles abre un diálogo para editar los parámetros del efecto.

Al hacer clic derecho aparecerá un menú contextual donde se puede cambiar el orden en que se procesan los efectos o eliminar un efecto completo.</translation>
    </message>
    <message>
        <source>Move &amp;up</source>
        <translation>Mover &amp;arriba</translation>
    </message>
    <message>
        <source>Move &amp;down</source>
        <translation></translation>
    </message>
    <message>
        <source>&amp;Remove this plugin</source>
        <translation>&amp;Remover este complemento</translation>
    </message>
    <message>
        <source>&amp;Help</source>
        <translation>&amp;Ayuda</translation>
    </message>
</context>
<context>
    <name>EnvelopeAndLfoParameters</name>
    <message>
        <source>Predelay</source>
        <translation>Pre retraso</translation>
    </message>
    <message>
        <source>Attack</source>
        <translation>Ataque</translation>
    </message>
    <message>
        <source>Hold</source>
        <translation>Hold</translation>
    </message>
    <message>
        <source>Decay</source>
        <translation>Decay</translation>
    </message>
    <message>
        <source>Sustain</source>
        <translation>Sustain</translation>
    </message>
    <message>
        <source>Release</source>
        <translation>Release</translation>
    </message>
    <message>
        <source>Modulation</source>
        <translation>Modulación</translation>
    </message>
    <message>
        <source>LFO Predelay</source>
        <translation>LFO Predelay</translation>
    </message>
    <message>
        <source>LFO Attack</source>
        <translation>LFO Ataque</translation>
    </message>
    <message>
        <source>LFO speed</source>
        <translation>LFO velocidad</translation>
    </message>
    <message>
        <source>LFO Modulation</source>
        <translation>LFO Modulación</translation>
    </message>
    <message>
        <source>LFO Wave Shape</source>
        <translation>LFO Forma de onda</translation>
    </message>
    <message>
        <source>Freq x 100</source>
        <translation>Freq x 100</translation>
    </message>
    <message>
        <source>Modulate Env-Amount</source>
        <translation>Modulación Env-Amount</translation>
    </message>
</context>
<context>
    <name>EnvelopeAndLfoView</name>
    <message>
        <source>DEL</source>
        <translation>DEL</translation>
    </message>
    <message>
        <source>Predelay:</source>
        <translation>Pre retraso:</translation>
    </message>
    <message>
        <source>Use this knob for setting predelay of the current envelope. The bigger this value the longer the time before start of actual envelope.</source>
        <translation>Use este control para configurar el pre-retraso de la envolvente actual. A mayor valor mayor el tiempo antes del inicio de la envolvente actual.</translation>
    </message>
    <message>
        <source>ATT</source>
        <translation>ATT</translation>
    </message>
    <message>
        <source>Attack:</source>
        <translation>Ataque:</translation>
    </message>
    <message>
        <source>Use this knob for setting attack-time of the current envelope. The bigger this value the longer the envelope needs to increase to attack-level. Choose a small value for instruments like pianos and a big value for strings.</source>
        <translation>Use este control para configurar el tiempo de ataque de la envolvente actual. A mayor valor mayor tiempo necesitará la envolvente para alcanzar el nivel de ataque. Escoja un valor pequeño para instrumentos como pianos y alto para cuerdas.</translation>
    </message>
    <message>
        <source>HOLD</source>
        <translation>HOLD</translation>
    </message>
    <message>
        <source>Hold:</source>
        <translation>Hold:</translation>
    </message>
    <message>
        <source>Use this knob for setting hold-time of the current envelope. The bigger this value the longer the envelope holds attack-level before it begins to decrease to sustain-level.</source>
        <translation>Use este control para configurar el tiempo de mantenimiento de la envolvente actual. A mayor valor mayor tiempo se mantendrá el nivel de ataque hasta que comience a disminuir hasta el nivel de sostenido.</translation>
    </message>
    <message>
        <source>DEC</source>
        <translation>DEC</translation>
    </message>
    <message>
        <source>Decay:</source>
        <translation>Decay:</translation>
    </message>
    <message>
        <source>Use this knob for setting decay-time of the current envelope. The bigger this value the longer the envelope needs to decrease from attack-level to sustain-level. Choose a small value for instruments like pianos.</source>
        <translation>Use este control para configurar el tiempo de caída de la envolvente actual. A mayor valor mayor tiempo necesitará la envolvente para pasar del nivel de ataque a sostenido. Escoja un valor pequeño para instrumentos como pianos.</translation>
    </message>
    <message>
        <source>SUST</source>
        <translation>SUST</translation>
    </message>
    <message>
        <source>Sustain:</source>
        <translation>Sustain:</translation>
    </message>
    <message>
        <source>Use this knob for setting sustain-level of the current envelope. The bigger this value the higher the level on which the envelope stays before going down to zero.</source>
        <translation>Use este control para configurar el nivel de sostenido de la envolvente actual. A mayor valor mayor tiempo tardará la envolvente hasta llegar a cero.</translation>
    </message>
    <message>
        <source>REL</source>
        <translation>REL</translation>
    </message>
    <message>
        <source>Release:</source>
        <translation>Release:</translation>
    </message>
    <message>
        <source>Use this knob for setting release-time of the current envelope. The bigger this value the longer the envelope needs to decrease from sustain-level to zero. Choose a big value for soft instruments like strings.</source>
        <translation>Use este control para configurar el intervalo de sostenido de la envolvente actual. A mayor valor, la envolvente necesitará más tiempo para pasar de sostenido a cero. Escoja un valor grande para instrumentos suaves como cuerdas.</translation>
    </message>
    <message>
        <source>AMT</source>
        <translation>AMT</translation>
    </message>
    <message>
        <source>Modulation amount:</source>
        <translation>Cantidad de modulación:</translation>
    </message>
    <message>
        <source>Use this knob for setting modulation amount of the current envelope. The bigger this value the more the according size (e.g. volume or cutoff-frequency) will be influenced by this envelope.</source>
        <translation>Use este control para configurar la cantidad de modulación de la envolvente actual. A mayor valor mayor valor de acorde (ejemplo volumen o corte de frecuencia) será influenciado por esta envolvente.</translation>
    </message>
    <message>
        <source>LFO predelay:</source>
        <translation>LFO Pre retraso:</translation>
    </message>
    <message>
        <source>Use this knob for setting predelay-time of the current LFO. The bigger this value the the time until the LFO starts to oscillate.</source>
        <translation>Use este control para configurar el pre-retraso del LFO actual. A mayor valor, mayor tiempo hasta que el LFO comience a oscilar.</translation>
    </message>
    <message>
        <source>LFO- attack:</source>
        <translation>LFO Ataque:</translation>
    </message>
    <message>
        <source>Use this knob for setting attack-time of the current LFO. The bigger this value the longer the LFO needs to increase its amplitude to maximum.</source>
        <translation>Use este control para configurar el tiempo de ataque del LFO actual. A mayor valor mayor tiempo necesitara el LFO para aumentar su amplitud al máximo.</translation>
    </message>
    <message>
        <source>SPD</source>
        <translation>SPD</translation>
    </message>
    <message>
        <source>LFO speed:</source>
        <translation>LFO velocidad:</translation>
    </message>
    <message>
        <source>Use this knob for setting speed of the current LFO. The bigger this value the faster the LFO oscillates and the faster will be your effect.</source>
        <translation>Use este control para configurar la velocidad del LFO actual: A mayor valor más rápido oscilará el LFO y mayor será el efecto.</translation>
    </message>
    <message>
        <source>Use this knob for setting modulation amount of the current LFO. The bigger this value the more the selected size (e.g. volume or cutoff-frequency) will be influenced by this LFO.</source>
        <translation>Use este control para configurar la modulación del actual LFO. A mayor valor mayor cantidad seleccionada (por ejemplo de volumen o frecuencia de corte) será influenciado por este LFO.</translation>
    </message>
    <message>
        <source>Click here for a sine-wave.</source>
        <translation>Clic acá para una onda sinusoidal.</translation>
    </message>
    <message>
        <source>Click here for a triangle-wave.</source>
        <translation>Clic acá para una onda triangular.</translation>
    </message>
    <message>
        <source>Click here for a saw-wave for current.</source>
        <translation>Clic acá para obtener una onda de sierra.</translation>
    </message>
    <message>
        <source>Click here for a square-wave.</source>
        <translation>Clic acá para una onda cuadrada.</translation>
    </message>
    <message>
        <source>Click here for a user-defined wave. Afterwards, drag an according sample-file onto the LFO graph.</source>
        <translation>Clic acá para una onda definida por el usuario. Después, arrastre una muestra de archivo de acuerdo a la gráfica LFO.</translation>
    </message>
    <message>
        <source>FREQ x 100</source>
        <translation>FREQ x 100</translation>
    </message>
    <message>
        <source>Click here if the frequency of this LFO should be multiplied by 100.</source>
        <translation>Hacé clic acá si la frecuencia de esta LFO debe ser multiplicada por 100.</translation>
    </message>
    <message>
        <source>multiply LFO-frequency by 100</source>
        <translation>Multiplicar la frecuencia LFO por 100</translation>
    </message>
    <message>
        <source>MODULATE ENV-AMOUNT</source>
        <translation>Modular cantidad de envolvente</translation>
    </message>
    <message>
        <source>Click here to make the envelope-amount controlled by this LFO.</source>
        <translation>Click aquí para crear la envolvente controlada por este LFO.</translation>
    </message>
    <message>
        <source>control envelope-amount by this LFO</source>
        <translation>controla la cantidad de envolvente para este LFO</translation>
    </message>
    <message>
        <source>ms/LFO:</source>
        <translation>ms/LFO:</translation>
    </message>
    <message>
        <source>Hint</source>
        <translation>TIP</translation>
    </message>
    <message>
        <source>Drag a sample from somewhere and drop it in this window.</source>
        <translation>Arrastre una muestra desde algún sitio y colóquela en esta ventana.</translation>
    </message>
</context>
<context>
    <name>ExportProjectDialog</name>
    <message>
        <source>Export project</source>
        <translation>Exportar proyecto</translation>
    </message>
    <message>
        <source>Output</source>
        <translation>Salida</translation>
    </message>
    <message>
        <source>File format:</source>
        <translation>Formato de archivo:</translation>
    </message>
    <message>
        <source>Samplerate:</source>
        <translation>Frecuencia de muestreo:</translation>
    </message>
    <message>
        <source>44100 Hz</source>
        <translation>44100 Hz</translation>
    </message>
    <message>
        <source>48000 Hz</source>
        <translation>48000 Hz</translation>
    </message>
    <message>
        <source>88200 Hz</source>
        <translation>88200 Hz</translation>
    </message>
    <message>
        <source>96000 Hz</source>
        <translation>96000 Hz</translation>
    </message>
    <message>
        <source>192000 Hz</source>
        <translation>192000 Hz</translation>
    </message>
    <message>
        <source>Bitrate:</source>
        <translation>Tasa de bits:</translation>
    </message>
    <message>
        <source>64 KBit/s</source>
        <translation>64 KBit/s</translation>
    </message>
    <message>
        <source>128 KBit/s</source>
        <translation>128 KBit/s</translation>
    </message>
    <message>
        <source>160 KBit/s</source>
        <translation>160 KBit/s</translation>
    </message>
    <message>
        <source>192 KBit/s</source>
        <translation>192 KBit/s</translation>
    </message>
    <message>
        <source>256 KBit/s</source>
        <translation>256 KBit/s</translation>
    </message>
    <message>
        <source>320 KBit/s</source>
        <translation>320 KBit/s</translation>
    </message>
    <message>
        <source>Depth:</source>
        <translation>Profundidad:</translation>
    </message>
    <message>
        <source>16 Bit Integer</source>
        <translation>Entero 16 Bit</translation>
    </message>
    <message>
        <source>32 Bit Float</source>
        <translation>Flotante 32 Bit</translation>
    </message>
    <message>
        <source>Please note that not all of the parameters above apply for all file formats.</source>
        <translation>Tenga en cuenta que no todos los parámetros anteriores se aplican para todos los formatos de archivo.</translation>
    </message>
    <message>
        <source>Quality settings</source>
        <translation>Ajustes de Calidad</translation>
    </message>
    <message>
        <source>Interpolation:</source>
        <translation>Interpolación:</translation>
    </message>
    <message>
        <source>Zero Order Hold</source>
        <translation>Zero Order Hold</translation>
    </message>
    <message>
        <source>Sinc Fastest</source>
        <translation>Sinc Rápido</translation>
    </message>
    <message>
        <source>Sinc Medium (recommended)</source>
        <translation>Sinc Medio (recomendado)</translation>
    </message>
    <message>
        <source>Sinc Best (very slow!)</source>
        <translation>Sinc Mejor (Muy lento!)</translation>
    </message>
    <message>
        <source>Oversampling (use with care!):</source>
        <translation>Sobremuestreo (utilizar con cuidado!):</translation>
    </message>
    <message>
        <source>1x (None)</source>
        <translation>1x (Nada)</translation>
    </message>
    <message>
        <source>2x</source>
        <translation>2x</translation>
    </message>
    <message>
        <source>4x</source>
        <translation>4x</translation>
    </message>
    <message>
        <source>8x</source>
        <translation>8x</translation>
    </message>
    <message>
        <source>Sample-exact controllers</source>
        <translation>Controladores de muestra exacta</translation>
    </message>
    <message>
        <source>Alias-free oscillators</source>
        <translation>Osciladores Alias free</translation>
    </message>
    <message>
        <source>Start</source>
        <translation>Iniciar</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation>Cancelar</translation>
    </message>
    <message>
        <source>Export as loop (remove end silence)</source>
        <translation>Exportar como bucle (eliminar el silencio final)</translation>
    </message>
</context>
<context>
    <name>FxMixer</name>
    <message>
        <source>Master</source>
        <translation>Maestro</translation>
    </message>
    <message>
        <source>FX %1</source>
        <translation>FX %1</translation>
    </message>
</context>
<context>
    <name>FxMixerView</name>
    <message>
        <source>Rename FX channel</source>
        <translation>Renombrar este canal FX</translation>
    </message>
    <message>
        <source>Enter the new name for this FX channel</source>
        <translation>Introduzca el nuevo nombre para este canal FX</translation>
    </message>
    <message>
        <source>FX-Mixer</source>
        <translation>Mezclador FX</translation>
    </message>
    <message>
        <source>FX Fader %1</source>
        <translation>Deslizador FX %1</translation>
    </message>
    <message>
        <source>Mute</source>
        <translation>Silenciar</translation>
    </message>
    <message>
        <source>Mute this FX channel</source>
        <translation>Silenciar este canal FX</translation>
    </message>
</context>
<context>
    <name>InstrumentFunctionArpeggio</name>
    <message>
        <source>Arpeggio</source>
        <translation>Arpegio</translation>
    </message>
    <message>
        <source>Arpeggio type</source>
        <translation>Tipo de Arpegio</translation>
    </message>
    <message>
        <source>Arpeggio range</source>
        <translation>Rango de Arpegio</translation>
    </message>
    <message>
        <source>Arpeggio time</source>
        <translation>Tiempo de Arpegio</translation>
    </message>
    <message>
        <source>Arpeggio gate</source>
        <translation>Puerto de Arpegio</translation>
    </message>
    <message>
        <source>Arpeggio direction</source>
        <translation>Dirección de Arpegio</translation>
    </message>
    <message>
        <source>Arpeggio mode</source>
        <translation>Modo de Arpegio</translation>
    </message>
    <message>
        <source>Up</source>
        <translation>Arriba</translation>
    </message>
    <message>
        <source>Down</source>
        <translation>Abajo</translation>
    </message>
    <message>
        <source>Up and down</source>
        <translation>Arriba y abajo</translation>
    </message>
    <message>
        <source>Random</source>
        <translation>Aleatorio</translation>
    </message>
    <message>
        <source>Free</source>
        <translation>Libre</translation>
    </message>
    <message>
        <source>Sort</source>
        <translation>Clase</translation>
    </message>
    <message>
        <source>Sync</source>
        <translation>Sincronizado</translation>
    </message>
</context>
<context>
    <name>InstrumentFunctionArpeggioView</name>
    <message>
        <source>ARPEGGIO</source>
        <translation>ARPEGIO</translation>
    </message>
    <message>
        <source>An arpeggio is a method playing (especially plucked) instruments, which makes the music much livelier. The strings of such instruments (e.g. harps) are plucked like chords. The only difference is that this is done in a sequential order, so the notes are not played at the same time. Typical arpeggios are major or minor triads, but there are a lot of other possible chords, you can select.</source>
        <translation>Un arpegio es un conjunto de tres o más sonidos musicales combinados armónicamente y tocados uno tras otro de manera más o menos rápida. Un acorde son varias notas tocadas en simultáneo, el arpegio difiere del acorde ya que las notas se tocan en un orden secuencial y no al mismo tiempo. Arpegios típicos son tríadas mayores o menores, pero hay un montón de otros acordes posibles que podés seleccionar.</translation>
    </message>
    <message>
        <source>RANGE</source>
        <translation>RANGO</translation>
    </message>
    <message>
        <source>Arpeggio range:</source>
        <translation>Rango de Arpegio:</translation>
    </message>
    <message>
        <source>octave(s)</source>
        <translation>octava(s)</translation>
    </message>
    <message>
        <source>Use this knob for setting the arpeggio range in octaves. The selected arpeggio will be played within specified number of octaves.</source>
        <translation>Usá este mando para ajustar el rango de arpegio en octavas. El arpegio seleccionado sonará dentro de  un determinado número de octavas.</translation>
    </message>
    <message>
        <source>TIME</source>
        <translation>TIEMPO</translation>
    </message>
    <message>
        <source>Arpeggio time:</source>
        <translation>Tiempo de Arpegio:</translation>
    </message>
    <message>
        <source>ms</source>
        <translation>ms</translation>
    </message>
    <message>
        <source>Use this knob for setting the arpeggio time in milliseconds. The arpeggio time specifies how long each arpeggio-tone should be played.</source>
        <translation>Use este control para ajustar el intervalo arpegio en milisegundos. El intervalo de arpegio indica cuanto durará cada tono del arpegio.</translation>
    </message>
    <message>
        <source>GATE</source>
        <translation>PUERTA</translation>
    </message>
    <message>
        <source>Arpeggio gate:</source>
        <translation>Puerto de Arpegio:</translation>
    </message>
    <message>
        <source>%</source>
        <translation>%</translation>
    </message>
    <message>
        <source>Use this knob for setting the arpeggio gate. The arpeggio gate specifies the percent of a whole arpeggio-tone that should be played. With this you can make cool staccato arpeggios.</source>
        <translation>Utilizá este mando para ajustar la puerta de arpegio. La puerta arpegio especifica el porcentaje de un arpegio-que se debe reproducir.</translation>
    </message>
    <message>
        <source>Chord:</source>
        <translation>Acorde:</translation>
    </message>
    <message>
        <source>Direction:</source>
        <translation>Dirección:</translation>
    </message>
    <message>
        <source>Mode:</source>
        <translation>Modo:</translation>
    </message>
</context>
<context>
    <name>InstrumentFunctionNoteStacking</name>
    <message>
        <source>octave</source>
        <translation>Octava</translation>
    </message>
    <message>
        <source>Major</source>
        <translation>Mayor</translation>
    </message>
    <message>
        <source>Majb5</source>
        <translation>Mayor5</translation>
    </message>
    <message>
        <source>minor</source>
        <translation>menor</translation>
    </message>
    <message>
        <source>minb5</source>
        <translation>menor5</translation>
    </message>
    <message>
        <source>sus2</source>
        <translation>sus2</translation>
    </message>
    <message>
        <source>sus4</source>
        <translation>sus4</translation>
    </message>
    <message>
        <source>aug</source>
        <translation>aug</translation>
    </message>
    <message>
        <source>augsus4</source>
        <translation>augsus4</translation>
    </message>
    <message>
        <source>tri</source>
        <translation>tri</translation>
    </message>
    <message>
        <source>6</source>
        <translation>6</translation>
    </message>
    <message>
        <source>6sus4</source>
        <translation>6sus4</translation>
    </message>
    <message>
        <source>6add9</source>
        <translation>madd9</translation>
    </message>
    <message>
        <source>m6</source>
        <translation>m6</translation>
    </message>
    <message>
        <source>m6add9</source>
        <translation>m6add9</translation>
    </message>
    <message>
        <source>7</source>
        <translation>7</translation>
    </message>
    <message>
        <source>7sus4</source>
        <translation>7sus4</translation>
    </message>
    <message>
        <source>7#5</source>
        <translation>7#5</translation>
    </message>
    <message>
        <source>7b5</source>
        <translation>7b5</translation>
    </message>
    <message>
        <source>7#9</source>
        <translation>7#9</translation>
    </message>
    <message>
        <source>7b9</source>
        <translation>7b9</translation>
    </message>
    <message>
        <source>7#5#9</source>
        <translation>7#5#9</translation>
    </message>
    <message>
        <source>7#5b9</source>
        <translation>7#5b9</translation>
    </message>
    <message>
        <source>7b5b9</source>
        <translation>7b5b9</translation>
    </message>
    <message>
        <source>7add11</source>
        <translation>7add11</translation>
    </message>
    <message>
        <source>7add13</source>
        <translation>7add13</translation>
    </message>
    <message>
        <source>7#11</source>
        <translation>7#11</translation>
    </message>
    <message>
        <source>Maj7</source>
        <translation>Maj7</translation>
    </message>
    <message>
        <source>Maj7b5</source>
        <translation>Maj7b5</translation>
    </message>
    <message>
        <source>Maj7#5</source>
        <translation>Maj7#5</translation>
    </message>
    <message>
        <source>Maj7#11</source>
        <translation>Maj7#11</translation>
    </message>
    <message>
        <source>Maj7add13</source>
        <translation>Maj7add13</translation>
    </message>
    <message>
        <source>m7</source>
        <translation>m7</translation>
    </message>
    <message>
        <source>m7b5</source>
        <translation>m7b5</translation>
    </message>
    <message>
        <source>m7b9</source>
        <translation>m7b9</translation>
    </message>
    <message>
        <source>m7add11</source>
        <translation>m7add11</translation>
    </message>
    <message>
        <source>m7add13</source>
        <translation>m7add13</translation>
    </message>
    <message>
        <source>m-Maj7</source>
        <translation>m-Maj7</translation>
    </message>
    <message>
        <source>m-Maj7add11</source>
        <translation>m-Maj7add11</translation>
    </message>
    <message>
        <source>m-Maj7add13</source>
        <translation>m-Maj7add13</translation>
    </message>
    <message>
        <source>9</source>
        <translation>9</translation>
    </message>
    <message>
        <source>9sus4</source>
        <translation>9sus4</translation>
    </message>
    <message>
        <source>add9</source>
        <translation>add9</translation>
    </message>
    <message>
        <source>9#5</source>
        <translation>9#5</translation>
    </message>
    <message>
        <source>9b5</source>
        <translation>9b5</translation>
    </message>
    <message>
        <source>9#11</source>
        <translation>9#11</translation>
    </message>
    <message>
        <source>9b13</source>
        <translation>9b13</translation>
    </message>
    <message>
        <source>Maj9</source>
        <translation>Maj9</translation>
    </message>
    <message>
        <source>Maj9sus4</source>
        <translation>Maj9sus4</translation>
    </message>
    <message>
        <source>Maj9#5</source>
        <translation>Maj9#5</translation>
    </message>
    <message>
        <source>Maj9#11</source>
        <translation>Maj9#11</translation>
    </message>
    <message>
        <source>m9</source>
        <translation>m9</translation>
    </message>
    <message>
        <source>madd9</source>
        <translation>madd9</translation>
    </message>
    <message>
        <source>m9b5</source>
        <translation>m9b5</translation>
    </message>
    <message>
        <source>m9-Maj7</source>
        <translation>m9-Maj7</translation>
    </message>
    <message>
        <source>11</source>
        <translation>11</translation>
    </message>
    <message>
        <source>11b9</source>
        <translation>11b9</translation>
    </message>
    <message>
        <source>Maj11</source>
        <translation>Maj11</translation>
    </message>
    <message>
        <source>m11</source>
        <translation>m11</translation>
    </message>
    <message>
        <source>m-Maj11</source>
        <translation>m-Maj11</translation>
    </message>
    <message>
        <source>13</source>
        <translation>13</translation>
    </message>
    <message>
        <source>13#9</source>
        <translation>13#9</translation>
    </message>
    <message>
        <source>13b9</source>
        <translation>13b9</translation>
    </message>
    <message>
        <source>13b5b9</source>
        <translation>13b5b9</translation>
    </message>
    <message>
        <source>Maj13</source>
        <translation>Maj13</translation>
    </message>
    <message>
        <source>m13</source>
        <translation>m13</translation>
    </message>
    <message>
        <source>m-Maj13</source>
        <translation>m-Maj13</translation>
    </message>
    <message>
        <source>Harmonic minor</source>
        <translation>Menor armónico</translation>
    </message>
    <message>
        <source>Melodic minor</source>
        <translation>Menor de la melodía</translation>
    </message>
    <message>
        <source>Whole tone</source>
        <translation>Tono entero</translation>
    </message>
    <message>
        <source>Diminished</source>
        <translation>Disminuido</translation>
    </message>
    <message>
        <source>Major pentatonic</source>
        <translation>Mayor pentatónico</translation>
    </message>
    <message>
        <source>Minor pentatonic</source>
        <translation>Menor pentatónico</translation>
    </message>
    <message>
        <source>Jap in sen</source>
        <translation>Jap en sen</translation>
    </message>
    <message>
        <source>Major bebop</source>
        <translation>Mayor Bebop</translation>
    </message>
    <message>
        <source>Dominant bebop</source>
        <translation>Bebop dominante</translation>
    </message>
    <message>
        <source>Blues</source>
        <translation>Blues</translation>
    </message>
    <message>
        <source>Arabic</source>
        <translation>Árabe</translation>
    </message>
    <message>
        <source>Enigmatic</source>
        <translation>Enigmatico</translation>
    </message>
    <message>
        <source>Neopolitan</source>
        <translation>Neopolita</translation>
    </message>
    <message>
        <source>Neopolitan minor</source>
        <translation>Menor Neopolita</translation>
    </message>
    <message>
        <source>Hungarian minor</source>
        <translation>Menor Húngaro</translation>
    </message>
    <message>
        <source>Dorian</source>
        <translation>Dorian</translation>
    </message>
    <message>
        <source>Phrygolydian</source>
        <translation>Phrygolydian</translation>
    </message>
    <message>
        <source>Lydian</source>
        <translation>Lidio</translation>
    </message>
    <message>
        <source>Mixolydian</source>
        <translation>Mixolydian</translation>
    </message>
    <message>
        <source>Aeolian</source>
        <translation>Aeolian</translation>
    </message>
    <message>
        <source>Locrian</source>
        <translation>Locrian</translation>
    </message>
    <message>
        <source>Chords</source>
        <translation>Acordes</translation>
    </message>
    <message>
        <source>Chord type</source>
        <translation>Tipo de acorde</translation>
    </message>
    <message>
        <source>Chord range</source>
        <translation>Rango de acorde</translation>
    </message>
    <message>
        <source>Minor</source>
        <translation>Menor</translation>
    </message>
</context>
<context>
    <name>InstrumentFunctionNoteStackingView</name>
    <message>
        <source>RANGE</source>
        <translation>RANGO</translation>
    </message>
    <message>
        <source>Chord range:</source>
        <translation>Rango de acorde:</translation>
    </message>
    <message>
        <source>octave(s)</source>
        <translation>octava(s)</translation>
    </message>
    <message>
        <source>Use this knob for setting the chord range in octaves. The selected chord will be played within specified number of octaves.</source>
        <translation>Usá este mando para ajustar el rango acorde en octavas. El acorde seleccionado sonará dentro de un  determinado número de octavas.</translation>
    </message>
    <message>
        <source>STACKING</source>
        <translation>APILAMIENTO</translation>
    </message>
    <message>
        <source>Chord:</source>
        <translation>Acorde:</translation>
    </message>
</context>
<context>
    <name>InstrumentMidiIOView</name>
    <message>
        <source>ENABLE MIDI INPUT</source>
        <translation>Activar entrada MIDI</translation>
    </message>
    <message>
        <source>CHANNEL</source>
        <translation>CANAL</translation>
    </message>
    <message>
        <source>VELOCITY</source>
        <translation>VELOCIDAD</translation>
    </message>
    <message>
        <source>ENABLE MIDI OUTPUT</source>
        <translation>Activar salida MIDI</translation>
    </message>
    <message>
        <source>PROGRAM</source>
        <translation>PROGRAMA</translation>
    </message>
    <message>
        <source>MIDI devices to receive MIDI events from</source>
        <translation>Dispositivos MIDI desde los cuales recibir eventos MIDI</translation>
    </message>
    <message>
        <source>MIDI devices to send MIDI events to</source>
        <translation>Dispositivos MIDI a los cuales enviar eventos MIDI</translation>
    </message>
    <message>
        <source>NOTE</source>
        <translation>NOTA</translation>
    </message>
</context>
<context>
    <name>InstrumentSoundShaping</name>
    <message>
        <source>VOLUME</source>
        <translation>VOLUMEN</translation>
    </message>
    <message>
        <source>Volume</source>
        <translation>Volumen</translation>
    </message>
    <message>
        <source>CUTOFF</source>
        <translation>CORTE</translation>
    </message>
    <message>
        <source>Cutoff frequency</source>
        <translation>Corte de Frecuencia</translation>
    </message>
    <message>
        <source>RESO</source>
        <translation>RESO</translation>
    </message>
    <message>
        <source>Resonance</source>
        <translation>Resonancia</translation>
    </message>
    <message>
        <source>Envelopes/LFOs</source>
        <translation>Envolventes/LFOs</translation>
    </message>
    <message>
        <source>Filter type</source>
        <translation>Tipo de filtro</translation>
    </message>
    <message>
        <source>Q/Resonance</source>
        <translation>Q/Resonancia</translation>
    </message>
    <message>
        <source>LowPass</source>
        <translation>PasoBajo</translation>
    </message>
    <message>
        <source>HiPass</source>
        <translation>PasoAlto</translation>
    </message>
    <message>
        <source>BandPass csg</source>
        <translation>PasoBanda csg</translation>
    </message>
    <message>
        <source>BandPass czpg</source>
        <translation>PasoBanda czpg</translation>
    </message>
    <message>
        <source>Notch</source>
        <translation>Notch</translation>
    </message>
    <message>
        <source>Allpass</source>
        <translation>PasaTodo</translation>
    </message>
    <message>
        <source>Moog</source>
        <translation>Moog</translation>
    </message>
    <message>
        <source>2x LowPass</source>
        <translation>2x PasoBajo</translation>
    </message>
    <message>
        <source>RC LowPass 12dB</source>
        <translation>RC PasaBajos 12db</translation>
    </message>
    <message>
        <source>RC BandPass 12dB</source>
        <translation>RC Pasabanda 12db</translation>
    </message>
    <message>
        <source>RC HighPass 12dB</source>
        <translation>RC PasaAltos 12db</translation>
    </message>
    <message>
        <source>RC LowPass 24dB</source>
        <translation>RC PasaBajos 24db</translation>
    </message>
    <message>
        <source>RC BandPass 24dB</source>
        <translation>RC Pasabanda 24db</translation>
    </message>
    <message>
        <source>RC HighPass 24dB</source>
        <translation>RC PasaAltos 24db</translation>
    </message>
    <message>
        <source>Vocal Formant Filter</source>
        <translation>Filtro vocal Formant</translation>
    </message>
</context>
<context>
    <name>InstrumentSoundShapingView</name>
    <message>
        <source>TARGET</source>
        <translation>OBJETIVO</translation>
    </message>
    <message>
        <source>These tabs contain envelopes. They&apos;re very important for modifying a sound, in that they are almost always necessary for substractive synthesis. For example if you have a volume envelope, you can set when the sound should have a specific volume. If you want to create some soft strings then your sound has to fade in and out very softly. This can be done by setting large attack and release times. It&apos;s the same for other envelope targets like panning, cutoff frequency for the used filter and so on. Just monkey around with it! You can really make cool sounds out of a saw-wave with just some envelopes...!</source>
        <translation>Estas solapas contienen los envolventes. Son muy importantes para la modificación de un sonido, en que son casi siempre es necesario para la síntesis sustractiva. Por ejemplo, con una envolvente de volumen podés establecer cuando el sonido debe tener un volumen específico. Es lo mismo para otros objetivos de envolvente como paneo, frecuencia de corte para el filtro usado y así sucesivamente. Probando vas a familiarizarte con los envolventes para obtener sonidos frescos!</translation>
    </message>
    <message>
        <source>FILTER</source>
        <translation>FILTRO</translation>
    </message>
    <message>
        <source>Here you can select the built-in filter you want to use for this instrument-track. Filters are very important for changing the characteristics of a sound.</source>
        <translation>Acá podés seleccionar el filtro incorporado que quieras usar en estapista-instrumento. Los filtros son muy importante para cambiar las características de un sonido.</translation>
    </message>
    <message>
        <source>Hz</source>
        <translation>Hz</translation>
    </message>
    <message>
        <source>Use this knob for setting the cutoff frequency for the selected filter. The cutoff frequency specifies the frequency for cutting the signal by a filter. For example a lowpass-filter cuts all frequencies above the cutoff frequency. A highpass-filter cuts all frequencies below cutoff frequency, and so on...</source>
        <translation>Usá este mando para ajustar la frecuencia de corte para el filtro seleccionado. Por ejemplo, un filtro pasaBajo corta todas las frecuencias por encima de la frecuencia de corte.y un filtro pasaAlto corta todas las frecuencias por debajo de la frecuencia de corte...</translation>
    </message>
    <message>
        <source>RESO</source>
        <translation>RESO</translation>
    </message>
    <message>
        <source>Resonance:</source>
        <translation>Resonancia:</translation>
    </message>
    <message>
        <source>Use this knob for setting Q/Resonance for the selected filter. Q/Resonance tells the filter how much it should amplify frequencies near Cutoff-frequency.</source>
        <translation>Usá este mando para ajustar Q / Resonancia para el filtro seleccionado. Q / Resonancia le dice al filtro cuánto debe amplificar las frecuencias cerca del Corte de Frecuencia.</translation>
    </message>
    <message>
        <source>FREQ</source>
        <translation>FREQ</translation>
    </message>
    <message>
        <source>cutoff frequency:</source>
        <translation>Frecuencia de corte:</translation>
    </message>
</context>
<context>
    <name>InstrumentTrack</name>
    <message>
        <source>unnamed_track</source>
        <translation>Pista sin nombre</translation>
    </message>
    <message>
        <source>Volume</source>
        <translation>Volumen</translation>
    </message>
    <message>
        <source>Panning</source>
        <translation>Paneo</translation>
    </message>
    <message>
        <source>Pitch</source>
        <translation>Pitch</translation>
    </message>
    <message>
        <source>FX channel</source>
        <translation>Canal FX</translation>
    </message>
    <message>
        <source>Default preset</source>
        <translation>configuración predeterminada</translation>
    </message>
    <message>
        <source>With this knob you can set the volume of the opened channel.</source>
        <translation>Con este control podés indicar el volumen del canal abierto.</translation>
    </message>
    <message>
        <source>Base note</source>
        <translation>Nota base</translation>
    </message>
    <message>
        <source>Pitch range</source>
        <translation>Rango de Pitch</translation>
    </message>
</context>
<context>
    <name>InstrumentTrackView</name>
    <message>
        <source>Volume</source>
        <translation>Volumen</translation>
    </message>
    <message>
        <source>Volume:</source>
        <translation>Volumen:</translation>
    </message>
    <message>
        <source>VOL</source>
        <translation>VOL</translation>
    </message>
    <message>
        <source>Panning</source>
        <translation>Paneo</translation>
    </message>
    <message>
        <source>Panning:</source>
        <translation>Paneo:</translation>
    </message>
    <message>
        <source>PAN</source>
        <translation>Paneo</translation>
    </message>
    <message>
        <source>MIDI</source>
        <translation>MIDI</translation>
    </message>
    <message>
        <source>Input</source>
        <translation>Entrada</translation>
    </message>
    <message>
        <source>Output</source>
        <translation>Salida</translation>
    </message>
</context>
<context>
    <name>InstrumentTrackWindow</name>
    <message>
        <source>GENERAL SETTINGS</source>
        <translation>Configuración General</translation>
    </message>
    <message>
        <source>Click here, if you want to save current channel settings in a preset-file. Later you can load this preset by double-clicking it in the preset-browser.</source>
        <translation>Click acá si querés guardar la configuración del canal en una plantilla. Después podrás cargarlo haciendo doble clic en el navegador de plantillas.</translation>
    </message>
    <message>
        <source>Instrument volume</source>
        <translation>Volumen de Instrumento</translation>
    </message>
    <message>
        <source>Volume:</source>
        <translation>Volumen:</translation>
    </message>
    <message>
        <source>VOL</source>
        <translation>VOL</translation>
    </message>
    <message>
        <source>Panning</source>
        <translation>Paneo</translation>
    </message>
    <message>
        <source>Panning:</source>
        <translation>Paneo:</translation>
    </message>
    <message>
        <source>PAN</source>
        <translation>Paneo</translation>
    </message>
    <message>
        <source>Pitch</source>
        <translation>Pitch</translation>
    </message>
    <message>
        <source>Pitch:</source>
        <translation>Pitch:</translation>
    </message>
    <message>
        <source>cents</source>
        <translation>cent</translation>
    </message>
    <message>
        <source>PITCH</source>
        <translation>Pitch</translation>
    </message>
    <message>
        <source>FX channel</source>
        <translation>Canal FX</translation>
    </message>
    <message>
        <source>ENV/LFO</source>
        <translation>ENV/LFO</translation>
    </message>
    <message>
        <source>FUNC</source>
        <translation>FUNC</translation>
    </message>
    <message>
        <source>FX</source>
        <translation>FX</translation>
    </message>
    <message>
        <source>MIDI</source>
        <translation>MIDI</translation>
    </message>
    <message>
        <source>Save preset</source>
        <translation>Guardar configuración predeterminada</translation>
    </message>
    <message>
        <source>XML preset file (*.xpf)</source>
        <translation>Archivo de configuración predeterminada XML (*.xml)</translation>
    </message>
    <message>
        <source>PLUGIN</source>
        <translation>COMPLEMENTO</translation>
    </message>
    <message>
        <source>Save current channel settings in a preset-file</source>
        <translation>Guardar ajustes del canal actual en un archivo</translation>
    </message>
    <message>
        <source>Pitch range (semitones)</source>
        <translation>Rango de Pitch (semitonos)</translation>
    </message>
    <message>
        <source>RANGE</source>
        <translation>RANGO</translation>
    </message>
</context>
<context>
    <name>LadspaControl</name>
    <message>
        <source>Link channels</source>
        <translation>Conectar Canales</translation>
    </message>
</context>
<context>
    <name>LadspaControlDialog</name>
    <message>
        <source>Link Channels</source>
        <translation>Vincular Canales</translation>
    </message>
    <message>
        <source>Channel </source>
        <translation>Canal</translation>
    </message>
</context>
<context>
    <name>LadspaControlView</name>
    <message>
        <source>Link channels</source>
        <translation>Conectar Canales</translation>
    </message>
    <message>
        <source>Value:</source>
        <translation>Valor:</translation>
    </message>
    <message>
        <source>Sorry, no help available.</source>
        <translation>No hay ayuda disponible.</translation>
    </message>
</context>
<context>
    <name>LadspaEffect</name>
    <message>
        <source>Effect</source>
        <translation>Efecto</translation>
    </message>
    <message>
        <source>Unknown LADSPA plugin %1 requested.</source>
        <translation>Pedido de complemento %1 LADSPA desconocido.</translation>
    </message>
</context>
<context>
    <name>LfoController</name>
    <message>
        <source>LFO Controller</source>
        <translation>Controlador LFO</translation>
    </message>
    <message>
        <source>Base value</source>
        <translation>Valor base</translation>
    </message>
    <message>
        <source>Oscillator speed</source>
        <translation>Velocidad de Oscilador</translation>
    </message>
    <message>
        <source>Oscillator amount</source>
        <translation>Cantidad de oscilador</translation>
    </message>
    <message>
        <source>Oscillator phase</source>
        <translation>Fase de Oscilador</translation>
    </message>
    <message>
        <source>Oscillator waveform</source>
        <translation>Forma de onda del oscilador</translation>
    </message>
    <message>
        <source>Frequency Multiplier</source>
        <translation>Multiplicador de Frecuencia</translation>
    </message>
</context>
<context>
    <name>LfoControllerDialog</name>
    <message>
        <source>LFO</source>
        <translation>LFO</translation>
    </message>
    <message>
        <source>LFO Controller</source>
        <translation>Controlador LFO</translation>
    </message>
    <message>
        <source>BASE</source>
        <translation>BASE</translation>
    </message>
    <message>
        <source>Base amount:</source>
        <translation>Cantidad de Base:</translation>
    </message>
    <message>
        <source>todo</source>
        <translation>ParaHacer</translation>
    </message>
    <message>
        <source>SPD</source>
        <translation>SPD</translation>
    </message>
    <message>
        <source>LFO-speed:</source>
        <translation>LFO-velocidad:</translation>
    </message>
    <message>
        <source>Use this knob for setting speed of the LFO. The bigger this value the faster the LFO oscillates and the faster the effect.</source>
        <translation>Use este control para configurar la velocidad del LFO actual: A mayor valor más rápido oscilará el LFO y mayor será el efecto.</translation>
    </message>
    <message>
        <source>AMT</source>
        <translation>AMT</translation>
    </message>
    <message>
        <source>Modulation amount:</source>
        <translation>Cantidad de modulación:</translation>
    </message>
    <message>
        <source>Use this knob for setting modulation amount of the LFO. The bigger this value, the more the connected control (e.g. volume or cutoff-frequency) will be influenced by the LFO.</source>
        <translation>Use este control para configurar la modulación del LFO. A mayor valor, mayor cantidad seleccionada (por ejemplo de volumen o frecuencia de corte) será influenciado por este LFO.</translation>
    </message>
    <message>
        <source>PHS</source>
        <translation>PHS</translation>
    </message>
    <message>
        <source>Phase offset:</source>
        <translation>Demasía de Fase:</translation>
    </message>
    <message>
        <source>degrees</source>
        <translation>grados</translation>
    </message>
    <message>
        <source>With this knob you can set the phase offset of the LFO. That means you can move the point within an oscillation where the oscillator begins to oscillate. For example if you have a sine-wave and have a phase-offset of 180 degrees the wave will first go down. It&apos;s the same with a square-wave.</source>
        <translation>Con este mando podés ajustar el desplazamiento de la fase del LFO. Esto significa que podés mover el punto dentro de una oscilación cuando el oscilador comienza a oscilar. Por ejemplo, si tenés una onda senoidal y definís un desplazamiento de 180 grados la onda comenzará bajando. Lo mismo con una onda cuadrada.</translation>
    </message>
    <message>
        <source>Click here for a sine-wave.</source>
        <translation>Clic acá para una onda sinusoidal.</translation>
    </message>
    <message>
        <source>Click here for a triangle-wave.</source>
        <translation>Clic acá para una onda triangular.</translation>
    </message>
    <message>
        <source>Click here for a saw-wave.</source>
        <translation>Clic acá para obtener una onda de sierra.</translation>
    </message>
    <message>
        <source>Click here for a square-wave.</source>
        <translation>Clic acá para una onda cuadrada.</translation>
    </message>
    <message>
        <source>Click here for a a moog saw-wave.</source>
        <translation>Clic acá para obtener una onda de sierra Moog.</translation>
    </message>
    <message>
        <source>Click here for an exponential wave.</source>
        <translation>Clic acá para una onda exponencial.</translation>
    </message>
    <message>
        <source>Click here for white-noise.</source>
        <translation>Clic acá para ruido blanco.</translation>
    </message>
    <message>
        <source>Click here for a user-defined shape.
Double click to pick a file.</source>
        <translation>Clic acá para una onda definida por el usuario. Doble clic para seleccionar un archivo.</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <source>Working directory</source>
        <translation>Carpeta de trabajo</translation>
    </message>
    <message>
        <source>The LMMS working directory %1 does not exist. Create it now? You can change the directory later via Edit -&gt; Settings.</source>
        <translation>La carpeta de trabajo %1 no existe. ¿Querés crearla ahora? Luego podés modificarla desde las configuraciones.</translation>
    </message>
    <message>
        <source>Could not save config-file</source>
        <translation>No se pudo guardar el archivo de configuración</translation>
    </message>
    <message>
        <source>Could not save configuration file %1. You&apos;re probably not permitted to write to this file.
Please make sure you have write-access to the file and try again.</source>
        <translation>No se pudo guardar el archivo de configuración %1. Es probable que no estés autorizado a escribir en este archivo.
Por favor, asegurate de que tenés acceso de escritura en el archivo y vuelvé a intentarlo.</translation>
    </message>
    <message>
        <source>&amp;Project</source>
        <translation>&amp;Proyecto</translation>
    </message>
    <message>
        <source>&amp;New</source>
        <translation>&amp;Nuevo</translation>
    </message>
    <message>
        <source>&amp;Open...</source>
        <translation>&amp;Abrir...</translation>
    </message>
    <message>
        <source>Recently opened projects</source>
        <translation>Proyectos abiertos recientemente</translation>
    </message>
    <message>
        <source>&amp;Save</source>
        <translation>&amp;Guardar</translation>
    </message>
    <message>
        <source>Save &amp;As...</source>
        <translation>Guardar &amp;Como...</translation>
    </message>
    <message>
        <source>Import...</source>
        <translation>Importar...</translation>
    </message>
    <message>
        <source>E&amp;xport...</source>
        <translation>E&amp;xportar...</translation>
    </message>
    <message>
        <source>&amp;Quit</source>
        <translation>&amp;Salir</translation>
    </message>
    <message>
        <source>&amp;Edit</source>
        <translation>&amp;Editar</translation>
    </message>
    <message>
        <source>Settings</source>
        <translation>Configuraciones</translation>
    </message>
    <message>
        <source>&amp;Tools</source>
        <translation>&amp;Herramientas</translation>
    </message>
    <message>
        <source>&amp;Help</source>
        <translation>&amp;Ayuda</translation>
    </message>
    <message>
        <source>Online help</source>
        <translation>Ayuda en línea</translation>
    </message>
    <message>
        <source>Help</source>
        <translation>Ayuda</translation>
    </message>
    <message>
        <source>What&apos;s this?</source>
        <translation>¿Qué es esto?</translation>
    </message>
    <message>
        <source>About</source>
        <translation>Acerca de</translation>
    </message>
    <message>
        <source>Create new project</source>
        <translation>Crear nuevo proyecto</translation>
    </message>
    <message>
        <source>Create new project from template</source>
        <translation>Crear nuevo proyecto a partir de una plantilla</translation>
    </message>
    <message>
        <source>Open existing project</source>
        <translation>Abrir proyecto existente</translation>
    </message>
    <message>
        <source>Recently opened project</source>
        <translation>Proyecto abierto recientemente</translation>
    </message>
    <message>
        <source>Save current project</source>
        <translation>Guardar el proyecto actual</translation>
    </message>
    <message>
        <source>Export current project</source>
        <translation>Exportar proyecto actual</translation>
    </message>
    <message>
        <source>Show/hide Song-Editor</source>
        <translation>Mostrar/Ocultar el Editor de Canción</translation>
    </message>
    <message>
        <source>By pressing this button, you can show or hide the Song-Editor. With the help of the Song-Editor you can edit song-playlist and specify when which track should be played. You can also insert and move samples (e.g. rap samples) directly into the playlist.</source>
        <translation>Al pulsar este botón, se puede mostrar u ocultar el Editor de Canción. Con la ayuda del Editor de Canción podés editar la estructura de la canción y especificar cuando se debe reproducir cada patrón o pista. También puede insertar y mover muestras (todo tipo de grabaciones incluyendo voces) directamente en la lista de reproducción.</translation>
    </message>
    <message>
        <source>Show/hide Beat+Bassline Editor</source>
        <translation>Mostrar/Ocultar el Editor de Ritmo-Línea Base</translation>
    </message>
    <message>
        <source>By pressing this button, you can show or hide the Beat+Bassline Editor. The Beat+Bassline Editor is needed for creating beats, and for opening, adding, and removing channels, and for cutting, copying and pasting beat and bassline-patterns, and for other things like that.</source>
        <translation>Al pulsar este botón, se puede mostrar u ocultar el Editor de Ritmo-LíneaBase. El Editor de Ritmo-LíneaBase es necesario para la creación de patrones de ritmo, y para la apertura, adición y eliminación de canales, etc.</translation>
    </message>
    <message>
        <source>Show/hide Piano-Roll</source>
        <translation>Mostrar/Ocultar el Piano-Roll</translation>
    </message>
    <message>
        <source>Click here to show or hide the Piano-Roll. With the help of the Piano-Roll you can edit melodies in an easy way.</source>
        <translation>Clic acá para mostrar u ocultar el Piano-Roll. Con la ayuda del Piano-Roll podés editar melodías de forma fácil.</translation>
    </message>
    <message>
        <source>Show/hide Automation Editor</source>
        <translation>Mostrar/Ocultar el Editor de Automatización</translation>
    </message>
    <message>
        <source>Click here to show or hide the Automation Editor. With the help of the Automation Editor you can edit dynamic values in an easy way.</source>
        <translation>Clic acá para mostrar u ocultar el Editor de Automatización. Con la ayuda del editor de automatización puede editar valores dinámicos de una manera fácil.</translation>
    </message>
    <message>
        <source>Show/hide FX Mixer</source>
        <translation>Mostrar/Ocultar Mezclador FX</translation>
    </message>
    <message>
        <source>Click here to show or hide the FX Mixer. The FX Mixer is a very powerful tool for managing effects for your song. You can insert effects into different effect-channels.</source>
        <translation>Clic acá para mostrar u ocultar el Mezclador FX. El Mezclador FX es una herramienta muy poderosa para la gestión de los efectos de tu canción. Podés insertar efectos en diferentes canales.</translation>
    </message>
    <message>
        <source>Show/hide project notes</source>
        <translation>Mostrar/Ocultar las notas del proyecto</translation>
    </message>
    <message>
        <source>Click here to show or hide the project notes window. In this window you can put down your project notes.</source>
        <translation>Clic acá para mostrar u ocultar las Notas del Proyecto. En esta ventana podés poner notas sobre tu trabajo para recordarlas o para que alguien más tenga referencias.</translation>
    </message>
    <message>
        <source>Show/hide controller rack</source>
        <translation>Mostrar/Ocultar el bastidor de Controladores</translation>
    </message>
    <message>
        <source>Untitled</source>
        <translation>Sin Título</translation>
    </message>
    <message>
        <source>LMMS %1</source>
        <translation>LMMS %1</translation>
    </message>
    <message>
        <source>Project not saved</source>
        <translation>Proyecto no guardado</translation>
    </message>
    <message>
        <source>The current project was modified since last saving. Do you want to save it now?</source>
        <translation>El proyecto actual ha sido modificado desde la última vez que se guardó. Querés guardarlo ahora?</translation>
    </message>
    <message>
        <source>Open project</source>
        <translation>Abrir Proyecto</translation>
    </message>
    <message>
        <source>Save project</source>
        <translation>Guardar proyecto</translation>
    </message>
    <message>
        <source>Help not available</source>
        <translation>No hay ayuda disponible</translation>
    </message>
    <message>
        <source>Currently there&apos;s no help available in LMMS.
Please visit http://lmms.sf.net/wiki for documentation on LMMS.</source>
        <translation>Actualmente no hay ayuda disponible en LMMS.
Por favor, visite http://lmms.sf.net/wiki para la documentación de LMMS.</translation>
    </message>
    <message>
        <source>My projects</source>
        <translation>Mis proyectos</translation>
    </message>
    <message>
        <source>My samples</source>
        <translation>Mis muestras (samples)</translation>
    </message>
    <message>
        <source>My presets</source>
        <translation>Mis pre-configuraciones</translation>
    </message>
    <message>
        <source>My home</source>
        <translation>Mi /home</translation>
    </message>
    <message>
        <source>My computer</source>
        <translation>Mi computadora</translation>
    </message>
    <message>
        <source>Root directory</source>
        <translation>Carpeta Raíz</translation>
    </message>
    <message>
        <source>Save as new &amp;version</source>
        <translation>Guardar como Nueva &amp;Versión</translation>
    </message>
    <message>
        <source>E&amp;xport tracks...</source>
        <translation>E&amp;xportar pistas...</translation>
    </message>
    <message>
        <source>LMMS (*.mmp *.mmpz)</source>
        <translation>LMMS (*.mmp *.mmpz)</translation>
    </message>
    <message>
        <source>LMMS Project (*.mmp *.mmpz);;LMMS Project Template (*.mpt)</source>
        <translation>LMMS Proyecto (*.mmp *.mmpz);;LMMS Plantilla Proyecto (*.mpt)</translation>
    </message>
    <message>
        <source>Version %1</source>
        <translation>Versión %1</translation>
    </message>
    <message>
        <source>Project recovery</source>
        <translation>Recuperar Proyecto</translation>
    </message>
    <message>
        <source>It looks like the last session did not end properly. Do you want to recover the project of this session?</source>
        <translation>Parece que la última sesión no terminó correctamente. ¿Querés recuperar el proyecto de esa sesión?</translation>
    </message>
    <message>
        <source>Configuration file</source>
        <translation>Archivo de configuración</translation>
    </message>
    <message>
        <source>Error while parsing configuration file at line %1:%2: %3</source>
        <translation>Error al analizar el archivo de configuración en la(s) línea(s) %1:%2:%3</translation>
    </message>
</context>
<context>
    <name>MeterDialog</name>
    <message>
        <source>Meter Numerator</source>
        <translation>Meter Numerador</translation>
    </message>
    <message>
        <source>Meter Denominator</source>
        <translation>Meter denominador</translation>
    </message>
    <message>
        <source>TIME SIG</source>
        <translation>TIME SIG</translation>
    </message>
</context>
<context>
    <name>MeterModel</name>
    <message>
        <source>Numerator</source>
        <translation>Numerador</translation>
    </message>
    <message>
        <source>Denominator</source>
        <translation>Denominador</translation>
    </message>
</context>
<context>
    <name>MidiAlsaRaw::setupWidget</name>
    <message>
        <source>DEVICE</source>
        <translation>DISPOSITIVO</translation>
    </message>
</context>
<context>
    <name>MidiAlsaSeq::setupWidget</name>
    <message>
        <source>DEVICE</source>
        <translation>DISPOSITIVO</translation>
    </message>
</context>
<context>
    <name>MidiController</name>
    <message>
        <source>MIDI Controller</source>
        <translation>Controlador MIDI</translation>
    </message>
    <message>
        <source>unnamed_midi_controller</source>
        <translation>Controlador MIDI sin nombre</translation>
    </message>
</context>
<context>
    <name>MidiImport</name>
    <message>
        <source>Setup incomplete</source>
        <translation>Configuración incompleta</translation>
    </message>
    <message>
        <source>You do not have set up a default soundfont in the settings dialog (Edit-&gt;Settings). Therefore no sound will be played back after importing this MIDI file. You should download a General MIDI soundfont, specify it in settings dialog and try again.</source>
        <translation>Usted no ha configurado una fuente de sonido por defecto en el diálogo de configuración (Edición-&gt; Configuración). Por lo tanto no hay sonido se reproducirá después de importar este archivo MIDI. Usted debe descargar una fuente de sonido General MIDI, especificarlo en el diálogo de configuración y vuelva a intentarlo.</translation>
    </message>
    <message>
        <source>You did not compile LMMS with support for SoundFont2 player, which is used to add default sound to imported MIDI files. Therefore no sound will be played back after importing this MIDI file.</source>
        <translation>Usted no compiló LMMS con soporte para reproductor SoundFont2, que se utiliza para agregar sonido por defecto a los archivos MIDI importados. Por lo tanto no hay sonido se reproducirá después de importar este archivo MIDI.</translation>
    </message>
</context>
<context>
    <name>MidiOss::setupWidget</name>
    <message>
        <source>DEVICE</source>
        <translation>DISPOSITIVO</translation>
    </message>
</context>
<context>
    <name>MidiPort</name>
    <message>
        <source>Input channel</source>
        <translation>Canal de entrada</translation>
    </message>
    <message>
        <source>Output channel</source>
        <translation>Canal de salida</translation>
    </message>
    <message>
        <source>Input controller</source>
        <translation>Controlador de entrada</translation>
    </message>
    <message>
        <source>Output controller</source>
        <translation>Controlador de salida</translation>
    </message>
    <message>
        <source>Fixed input velocity</source>
        <translation>Velocidad de entrada determinada</translation>
    </message>
    <message>
        <source>Fixed output velocity</source>
        <translation>Velocidad de salida determinada</translation>
    </message>
    <message>
        <source>Output MIDI program</source>
        <translation>Programa de salida MIDI</translation>
    </message>
    <message>
        <source>Receive MIDI-events</source>
        <translation>Recibir eventos MIDI</translation>
    </message>
    <message>
        <source>Send MIDI-events</source>
        <translation>Enviar eventos MIDI</translation>
    </message>
    <message>
        <source>Fixed output note</source>
        <translation>Nota de salida determinada</translation>
    </message>
</context>
<context>
    <name>OscillatorObject</name>
    <message>
        <source>Osc %1 volume</source>
        <translation>Osc. %1 Volumen</translation>
    </message>
    <message>
        <source>Osc %1 panning</source>
        <translation>Osc %1 encuadramiento</translation>
    </message>
    <message>
        <source>Osc %1 coarse detuning</source>
        <translation>Osc %1 desintonización gruesa</translation>
    </message>
    <message>
        <source>Osc %1 fine detuning left</source>
        <translation>Osc %1 desintonización fina izquierda</translation>
    </message>
    <message>
        <source>Osc %1 fine detuning right</source>
        <translation>Osc %1 desintonización fina derecha</translation>
    </message>
    <message>
        <source>Osc %1 phase-offset</source>
        <translation>Osc %1 fase de compensación</translation>
    </message>
    <message>
        <source>Osc %1 stereo phase-detuning</source>
        <translation>Osc %1 fase de desintonización stereo</translation>
    </message>
    <message>
        <source>Osc %1 wave shape</source>
        <translation>Osc %1 forma de onda</translation>
    </message>
    <message>
        <source>Modulation type %1</source>
        <translation>Modulación tipo %1</translation>
    </message>
    <message>
        <source>Osc %1 waveform</source>
        <translation>OSC %1 Forma de onda</translation>
    </message>
</context>
<context>
    <name>PatmanView</name>
    <message>
        <source>Open other patch</source>
        <translation>Abrir otro parche</translation>
    </message>
    <message>
        <source>Click here to open another patch-file. Loop and Tune settings are not reset.</source>
        <translation>Clic acá para abrir otro archivo de parche. Las configuraciones de bucle y afinación permanecen iguales.</translation>
    </message>
    <message>
        <source>Loop</source>
        <translation>Bucle</translation>
    </message>
    <message>
        <source>Loop mode</source>
        <translation>Modo de bucle</translation>
    </message>
    <message>
        <source>Here you can toggle the Loop mode. If enabled, PatMan will use the loop information available in the file.</source>
        <translation>Acá se puede activar el Modo bucle. Si se activa, PatMan va a usar la información sobre bucle disponible en el archivo.</translation>
    </message>
    <message>
        <source>Tune</source>
        <translation>Afinación</translation>
    </message>
    <message>
        <source>Tune mode</source>
        <translation>Modo de Afinación</translation>
    </message>
    <message>
        <source>Here you can toggle the Tune mode. If enabled, PatMan will tune the sample to match the note&apos;s frequency.</source>
        <translation>Acá se puede activar el Modo Afinación. Si se activa, PatMan va a afinar la muestra para que coincida con la frecuencia de la nota.</translation>
    </message>
    <message>
        <source>No file selected</source>
        <translation>No se seleccionó ningún archivo</translation>
    </message>
    <message>
        <source>Open patch file</source>
        <translation>Abrir archivo de parches</translation>
    </message>
    <message>
        <source>Patch-Files (*.pat)</source>
        <translation>Archivos-Parche (*.pat)</translation>
    </message>
</context>
<context>
    <name>PeakController</name>
    <message>
        <source>Peak Controller</source>
        <translation>Controlador de Pico</translation>
    </message>
    <message>
        <source>Peak Controller Bug</source>
        <translation>Bug Controlador de Pico</translation>
    </message>
    <message>
        <source>Due to a bug in older version of LMMS, the peak controllers may not be connect properly. Please ensure that peak controllers are connected properly and re-save this file. Sorry for any inconvenience caused.</source>
        <translation>Debido a un error en la versión anterior de LMMS, los controladores de pico no se pueden conectar correctamente. Disculpas por el inconveniente.</translation>
    </message>
</context>
<context>
    <name>PeakControllerDialog</name>
    <message>
        <source>PEAK</source>
        <translation>PICO</translation>
    </message>
    <message>
        <source>LFO Controller</source>
        <translation>Controlador LFO</translation>
    </message>
</context>
<context>
    <name>PeakControllerEffectControlDialog</name>
    <message>
        <source>BASE</source>
        <translation>BASE</translation>
    </message>
    <message>
        <source>Base amount:</source>
        <translation>Cantidad de Base:</translation>
    </message>
    <message>
        <source>Modulation amount:</source>
        <translation>Cantidad de modulación:</translation>
    </message>
    <message>
        <source>Attack:</source>
        <translation>Ataque:</translation>
    </message>
    <message>
        <source>Release:</source>
        <translation>Release:</translation>
    </message>
    <message>
        <source>AMNT</source>
        <translation>AMNT</translation>
    </message>
    <message>
        <source>MULT</source>
        <translation>MULT</translation>
    </message>
    <message>
        <source>Amount Multiplicator:</source>
        <translation>Cantidad de multiplicador:</translation>
    </message>
    <message>
        <source>ATCK</source>
        <translation>ATCK</translation>
    </message>
    <message>
        <source>DCAY</source>
        <translation>DCAY</translation>
    </message>
</context>
<context>
    <name>PeakControllerEffectControls</name>
    <message>
        <source>Base value</source>
        <translation>Valor base</translation>
    </message>
    <message>
        <source>Modulation amount</source>
        <translation>Cantidad de modulación</translation>
    </message>
    <message>
        <source>Mute output</source>
        <translation>Silenciar salida</translation>
    </message>
    <message>
        <source>Attack</source>
        <translation>Ataque</translation>
    </message>
    <message>
        <source>Release</source>
        <translation>Release</translation>
    </message>
    <message>
        <source>Abs Value</source>
        <translation>Valor ABS</translation>
    </message>
    <message>
        <source>Amount Multiplicator</source>
        <translation>Cantidad de multiplicador</translation>
    </message>
</context>
<context>
    <name>PianoView</name>
    <message>
        <source>Base note</source>
        <translation>Nota Base</translation>
    </message>
</context>
<context>
    <name>Plugin</name>
    <message>
        <source>Plugin not found</source>
        <translation>Complemento no encontrado</translation>
    </message>
    <message>
        <source>The plugin &quot;%1&quot; wasn&apos;t found or could not be loaded!
Reason: &quot;%2&quot;</source>
        <translation>El complemento &quot;%1&quot; no fue encontrado o no pudo ser cargado!
Motivo: &quot;%2&quot;</translation>
    </message>
    <message>
        <source>Error while loading plugin</source>
        <translation>Error al cargar el complemento</translation>
    </message>
    <message>
        <source>Failed to load plugin &quot;%1&quot;!</source>
        <translation>Error al cargar el complemento &quot;%1&quot;!</translation>
    </message>
</context>
<context>
    <name>ProjectRenderer</name>
    <message>
        <source>WAV-File (*.wav)</source>
        <translation>Archivo-WAV (*.wav)</translation>
    </message>
    <message>
        <source>Compressed OGG-File (*.ogg)</source>
        <translation>Archivo OGG comprimido (*.ogg)</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <source>C</source>
        <comment>Note name</comment>
        <translation>C</translation>
    </message>
    <message>
        <source>Db</source>
        <comment>Note name</comment>
        <translation>Db</translation>
    </message>
    <message>
        <source>C#</source>
        <comment>Note name</comment>
        <translation>C#</translation>
    </message>
    <message>
        <source>D</source>
        <comment>Note name</comment>
        <translation>D</translation>
    </message>
    <message>
        <source>Eb</source>
        <comment>Note name</comment>
        <translation>Eb</translation>
    </message>
    <message>
        <source>D#</source>
        <comment>Note name</comment>
        <translation>D#</translation>
    </message>
    <message>
        <source>E</source>
        <comment>Note name</comment>
        <translation>E</translation>
    </message>
    <message>
        <source>Fb</source>
        <comment>Note name</comment>
        <translation>Fb</translation>
    </message>
    <message>
        <source>Gb</source>
        <comment>Note name</comment>
        <translation>Gb</translation>
    </message>
    <message>
        <source>F#</source>
        <comment>Note name</comment>
        <translation>F#</translation>
    </message>
    <message>
        <source>G</source>
        <comment>Note name</comment>
        <translation>G</translation>
    </message>
    <message>
        <source>Ab</source>
        <comment>Note name</comment>
        <translation>Ab</translation>
    </message>
    <message>
        <source>G#</source>
        <comment>Note name</comment>
        <translation>G#</translation>
    </message>
    <message>
        <source>A</source>
        <comment>Note name</comment>
        <translation>A</translation>
    </message>
    <message>
        <source>Bb</source>
        <comment>Note name</comment>
        <translation>Bb</translation>
    </message>
    <message>
        <source>A#</source>
        <comment>Note name</comment>
        <translation>A#</translation>
    </message>
    <message>
        <source>B</source>
        <comment>Note name</comment>
        <translation>B</translation>
    </message>
</context>
<context>
    <name>QWidget</name>
    <message>
        <source>Name: </source>
        <translation>Nombre: </translation>
    </message>
    <message>
        <source>Maker: </source>
        <translation>Desarrollador:  </translation>
    </message>
    <message>
        <source>Copyright: </source>
        <translation>DerechoDeAutor:</translation>
    </message>
    <message>
        <source>Requires Real Time: </source>
        <translation>Tiempo real requerido: </translation>
    </message>
    <message>
        <source>Yes</source>
        <translation>Si</translation>
    </message>
    <message>
        <source>No</source>
        <translation>No</translation>
    </message>
    <message>
        <source>Real Time Capable: </source>
        <translation>Tiempo real capaz: </translation>
    </message>
    <message>
        <source>In Place Broken: </source>
        <translation>In Place Broken: </translation>
    </message>
    <message>
        <source>Channels In: </source>
        <translation>Canales de entrada: </translation>
    </message>
    <message>
        <source>Channels Out: </source>
        <translation>Canales de salida: </translation>
    </message>
    <message>
        <source>File: </source>
        <translation>Archivo: </translation>
    </message>
</context>
<context>
    <name>SampleBuffer</name>
    <message>
        <source>Open audio file</source>
        <translation>Abrir archivo de audio</translation>
    </message>
    <message>
        <source>All Audio-Files (*.wav *.ogg *.ds *.flac *.spx *.voc *.aif *.aiff *.au *.raw *.mp3)</source>
        <translation>Todos los formatos (*.wav *.ogg *.ds *.flac *.spx *.voc *.aif *.aiff *.au *.raw *.mp3)</translation>
    </message>
    <message>
        <source>Wave-Files (*.wav)</source>
        <translation>Archivos Wave (*.wav)</translation>
    </message>
    <message>
        <source>OGG-Files (*.ogg)</source>
        <translation>Archivos OGG (*.ogg)</translation>
    </message>
    <message>
        <source>DrumSynth-Files (*.ds)</source>
        <translation>Archivos BateríaSynth (*.ds)</translation>
    </message>
    <message>
        <source>FLAC-Files (*.flac)</source>
        <translation>Archivos FLAC (*.flac)</translation>
    </message>
    <message>
        <source>SPEEX-Files (*.spx)</source>
        <translation>Archivos SPEEX (*.spx)</translation>
    </message>
    <message>
        <source>MP3-Files (*.mp3)</source>
        <translation>Archivos MP3 (*.mp3)</translation>
    </message>
    <message>
        <source>VOC-Files (*.voc)</source>
        <translation>Archivos VOC (*.voc)</translation>
    </message>
    <message>
        <source>AIFF-Files (*.aif *.aiff)</source>
        <translation>Archivos AIFF (*.aif *.aiff)</translation>
    </message>
    <message>
        <source>AU-Files (*.au)</source>
        <translation>Archivos AU (*.au)</translation>
    </message>
    <message>
        <source>RAW-Files (*.raw)</source>
        <translation>Archivos RAW (*.raw)</translation>
    </message>
</context>
<context>
    <name>SampleTCOView</name>
    <message>
        <source>double-click to select sample</source>
        <translation>doble clic para seleccionar muestra</translation>
    </message>
    <message>
        <source>Delete (middle mousebutton)</source>
        <translation>Eliminar (botón medio del ratón)</translation>
    </message>
    <message>
        <source>Cut</source>
        <translation>Cortar</translation>
    </message>
    <message>
        <source>Copy</source>
        <translation>Copiar</translation>
    </message>
    <message>
        <source>Paste</source>
        <translation>Pegar</translation>
    </message>
    <message>
        <source>Mute/unmute (&lt;Ctrl&gt; + middle click)</source>
        <translatorcomment>enmudecer/desenmudecer (&lt;Ctrl&gt; + clic medio)</translatorcomment>
        <translation></translation>
    </message>
    <message>
        <source>Set/clear record</source>
        <translation>Definir / limpiar grabación</translation>
    </message>
</context>
<context>
    <name>SampleTrack</name>
    <message>
        <source>Sample track</source>
        <translation>Pista de ejemplo</translation>
    </message>
    <message>
        <source>Volume</source>
        <translation>Volumen</translation>
    </message>
</context>
<context>
    <name>SampleTrackView</name>
    <message>
        <source>Track volume</source>
        <translation>Volumen de la pista</translation>
    </message>
    <message>
        <source>Channel volume:</source>
        <translation>Volumen del canal:</translation>
    </message>
    <message>
        <source>VOL</source>
        <translation>VOL</translation>
    </message>
</context>
<context>
    <name>TempoSyncKnob</name>
    <message>
        <source>Tempo Sync</source>
        <translation>Sincronizado Tiempo</translation>
    </message>
    <message>
        <source>No Sync</source>
        <translation>No Sinc</translation>
    </message>
    <message>
        <source>Eight beats</source>
        <translation>Ocho golpes</translation>
    </message>
    <message>
        <source>Whole note</source>
        <translation>Nota Entera</translation>
    </message>
    <message>
        <source>Half note</source>
        <translation>Media nota</translation>
    </message>
    <message>
        <source>Quarter note</source>
        <translation>Cuarto de nota</translation>
    </message>
    <message>
        <source>8th note</source>
        <translation>8° nota</translation>
    </message>
    <message>
        <source>16th note</source>
        <translation>16° nota</translation>
    </message>
    <message>
        <source>32nd note</source>
        <translation>32° nota</translation>
    </message>
    <message>
        <source>Custom...</source>
        <translation>Personalizado...</translation>
    </message>
    <message>
        <source>&amp;Help</source>
        <translation>&amp;Ayuda</translation>
    </message>
    <message>
        <source>Custom </source>
        <translation>Personalizado</translation>
    </message>
    <message>
        <source>Synced to Eight Beats</source>
        <translation>Sincronizado a Ocho Golpes</translation>
    </message>
    <message>
        <source>Synced to Whole Note</source>
        <translation>Sincronizado a Nota Entera</translation>
    </message>
    <message>
        <source>Synced to Half Note</source>
        <translation>Sincronizado a media Nota</translation>
    </message>
    <message>
        <source>Synced to Quarter Note</source>
        <translation>Sincronizado a cuarto de Nota</translation>
    </message>
    <message>
        <source>Synced to 8th Note</source>
        <translation>Sincronizado al 8vo de Nota</translation>
    </message>
    <message>
        <source>Synced to 16th Note</source>
        <translation>Sincronizado al 16 de Nota</translation>
    </message>
    <message>
        <source>Synced to 32nd Note</source>
        <translation>Sincronizado al 32 de Nota</translation>
    </message>
</context>
<context>
    <name>TimeDisplayWidget</name>
    <message>
        <source>click to change time units</source>
        <translation>Clic para cambiar unidades de tiempo</translation>
    </message>
</context>
<context>
    <name>TrackContainer</name>
    <message>
        <source>Couldn&apos;t import file</source>
        <translation>No se puede importar el archivo</translation>
    </message>
    <message>
        <source>Couldn&apos;t find a filter for importing file %1.
You should convert this file into a format supported by LMMS using another software.</source>
        <translation>No se pudo encontrar un filtro para importar el archivo %1.
Debes convertir este archivo en un formato compatible con LMMS utilizando otro software, como por ejemplo Arista o Audacity.</translation>
    </message>
    <message>
        <source>Couldn&apos;t open file</source>
        <translation>No se puede abrir el archivo</translation>
    </message>
    <message>
        <source>Couldn&apos;t open file %1 for reading.
Please make sure you have read-permission to the file and the directory containing the file and try again!</source>
        <translation>No se puede abrir el archivo %1 para lectura.
Por favor asegurate de que tenés permiso de escritura sobre el archivo y sobre la carpeta que contiene al archivo e ¡intentá nuevamente!</translation>
    </message>
    <message>
        <source>Loading project...</source>
        <translation>Cargando proyecto...</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation>Cancelar</translation>
    </message>
    <message>
        <source>Please wait...</source>
        <translation>Por favor, espere...</translation>
    </message>
    <message>
        <source>Importing MIDI-file...</source>
        <translation>Importar archivo MIDI...</translation>
    </message>
    <message>
        <source>Importing FLP-file...</source>
        <translation>Importar archivo FLP...</translation>
    </message>
</context>
<context>
    <name>TripleOscillatorView</name>
    <message>
        <source>Use phase modulation for modulating oscillator 2 with oscillator 1</source>
        <translation>Usar modulación de fase para oscilador 2 con oscilador 1</translation>
    </message>
    <message>
        <source>Use amplitude modulation for modulating oscillator 2 with oscillator 1</source>
        <translation>Usar modulación de amplitud para oscilador 2 con oscilador 1</translation>
    </message>
    <message>
        <source>Mix output of oscillator 1 &amp; 2</source>
        <translation>Salida mezclada para oscilador 1 &amp; 2</translation>
    </message>
    <message>
        <source>Synchronize oscillator 1 with oscillator 2</source>
        <translation>Sincronizar oscilador 1 con oscilador 2</translation>
    </message>
    <message>
        <source>Use frequency modulation for modulating oscillator 2 with oscillator 1</source>
        <translation>Usar frecuencia de modulación para oscilador 2 con oscilador 1</translation>
    </message>
    <message>
        <source>Use phase modulation for modulating oscillator 3 with oscillator 2</source>
        <translation>Usar modulación de fase para oscilador 3 con oscilador 2</translation>
    </message>
    <message>
        <source>Use amplitude modulation for modulating oscillator 3 with oscillator 2</source>
        <translation>Usar modulación de amplitud para oscilador 3 con oscilador 2</translation>
    </message>
    <message>
        <source>Mix output of oscillator 2 &amp; 3</source>
        <translation>Salida mezclada para oscilador 2 &amp; 3</translation>
    </message>
    <message>
        <source>Synchronize oscillator 2 with oscillator 3</source>
        <translation>Sincronizar oscilador 2 con oscilador 3</translation>
    </message>
    <message>
        <source>Use frequency modulation for modulating oscillator 3 with oscillator 2</source>
        <translation>Usar frecuencia de modulación para oscilador 3 con oscilador 2</translation>
    </message>
    <message>
        <source>Osc %1 volume:</source>
        <translation>Osc %1 Volumen:</translation>
    </message>
    <message>
        <source>With this knob you can set the volume of oscillator %1. When setting a value of 0 the oscillator is turned off. Otherwise you can hear the oscillator as loud as you set it here.</source>
        <translation>Con este control usted puede establecer el volumen del oscilador %1. Al fijar un valor de 0 se apaga. De lo contrario usted podrá oir al oscilador tan alto como lo especifique aquí.</translation>
    </message>
    <message>
        <source>Osc %1 panning:</source>
        <translation>Osc %1 paneo:</translation>
    </message>
    <message>
        <source>With this knob you can set the panning of the oscillator %1. A value of -100 means 100% left and a value of 100 moves oscillator-output right.</source>
        <translation>Con este control usted podrá establecer el encuadramiento del oscilador %1. Un valor de -100 significa 100% a la izquierda y un valor de 100 mueve el oscilador totalmente a la derecha.</translation>
    </message>
    <message>
        <source>Osc %1 coarse detuning:</source>
        <translation>Osc %1 desintonización gruesa:</translation>
    </message>
    <message>
        <source>semitones</source>
        <translation>semitonos</translation>
    </message>
    <message>
        <source>With this knob you can set the coarse detuning of oscillator %1. You can detune the oscillator 12 semitones (1 octave) up and down. This is useful for creating sounds with a chord.</source>
        <translation>Con este control podés establecer la desintonización gruesa del oscilador %1. Podés desintonizar el oscilador 12 semitonos (1 octava) arriba y abajo. Esto es útil para la creación de sonidos con un acorde.</translation>
    </message>
    <message>
        <source>Osc %1 fine detuning left:</source>
        <translation>Osc %1 desintonización fina izquierda:</translation>
    </message>
    <message>
        <source>cents</source>
        <translation>cents</translation>
    </message>
    <message>
        <source>With this knob you can set the fine detuning of oscillator %1 for the left channel. The fine-detuning is ranged between -100 cents and +100 cents. This is useful for creating &quot;fat&quot; sounds.</source>
        <translation>Con este control podés establecer la desintonización fina del oscilador %1 para el canal izquierdo 
La desintonización fina esta comprendida entre -100 cents y +100 cents. Ésto es útil para la creación de sonidos \&quot;gordos\&quot;.</translation>
    </message>
    <message>
        <source>Osc %1 fine detuning right:</source>
        <translation>Osc %1 desintonización fina derecha:</translation>
    </message>
    <message>
        <source>With this knob you can set the fine detuning of oscillator %1 for the right channel. The fine-detuning is ranged between -100 cents and +100 cents. This is useful for creating &quot;fat&quot; sounds.</source>
        <translation>Con este control podés establecer la desintonización fina del oscilador %1 para el canal derecho. La desintonización fina esta comprendida entre -100 cents y +100 cents. Esto es útil para la creación de sonidos \&quot;gordos\&quot;.</translation>
    </message>
    <message>
        <source>Osc %1 phase-offset:</source>
        <translation>Osc %1 fase de compensación:</translation>
    </message>
    <message>
        <source>degrees</source>
        <translation>grados</translation>
    </message>
    <message>
        <source>With this knob you can set the phase-offset of oscillator %1. That means you can move the point within an oscillation where the oscillator begins to oscillate. For example if you have a sine-wave and have a phase-offset of 180 degrees the wave will first go down. It&apos;s the same with a square-wave.</source>
        <translation>Con este control usted podrá establecer la fase de compensación del oscilador %1. Esto significa que usted puede mover el punto dentro de una oscilación donde el oscilador comienza a oscilar. Por ejemplo si usted tiene una onda senoidal y tiene una fase de compensación de 180 grados, la onda ira primero abajo. Lo mismo sucede con una onda cuadrada.</translation>
    </message>
    <message>
        <source>Osc %1 stereo phase-detuning:</source>
        <translation>Osc %1 fase de desintonización stereo:</translation>
    </message>
    <message>
        <source>With this knob you can set the stereo phase-detuning of oscillator %1. The stereo phase-detuning specifies the size of the difference between the phase-offset of left and right channel. This is very good for creating wide stereo sounds.</source>
        <translation>Con este control podés definir la fase estereo de desafinación para el oscilador %1. La fase estereo de desafinación especifica el tamaño de la diferencia entre la face de corte de los canales izquierdo y derecho. Esto es muy bueno para crear sonidos estereo amplios.</translation>
    </message>
    <message>
        <source>Use a sine-wave for current oscillator.</source>
        <translation>Utilice una onda sinusoidal para el oscilador actual.</translation>
    </message>
    <message>
        <source>Use a triangle-wave for current oscillator.</source>
        <translation>Utilice una onda triangular para el oscilador actual.</translation>
    </message>
    <message>
        <source>Use a saw-wave for current oscillator.</source>
        <translation>Utilice una onda de sierra para el oscilador actual.</translation>
    </message>
    <message>
        <source>Use a square-wave for current oscillator.</source>
        <translation>Utilice una onda cuadradal para el oscilador actual.</translation>
    </message>
    <message>
        <source>Use a moog-like saw-wave for current oscillator.</source>
        <translation>Utilice una onda de sierra moog para el oscilador actual.</translation>
    </message>
    <message>
        <source>Use an exponential wave for current oscillator.</source>
        <translation>Utilice una onda exponencial para el oscilador actual.</translation>
    </message>
    <message>
        <source>Use white-noise for current oscillator.</source>
        <translation>Utilice ruido blanco para el oscilador actual.</translation>
    </message>
    <message>
        <source>Use a user-defined waveform for current oscillator.</source>
        <translation>Utilice una onda definida por el usuario para el oscilador actual.</translation>
    </message>
</context>
<context>
    <name>Ui</name>
    <message>
        <source>Contributors ordered by number of commits:</source>
        <translation>Contribuidores ordenados por número de aportes:</translation>
    </message>
    <message>
        <source>Involved</source>
        <translation>Involucrado</translation>
    </message>
</context>
<context>
    <name>VersionedSaveDialog</name>
    <message>
        <source>Increment version number</source>
        <translation>Incrementar número de versión</translation>
    </message>
    <message>
        <source>Decrement version number</source>
        <translation>Decrementar número de versión</translation>
    </message>
</context>
<context>
    <name>VestigeInstrumentView</name>
    <message>
        <source>Open other VST-plugin</source>
        <translation>Abrir otro complemento VST</translation>
    </message>
    <message>
        <source>Click here, if you want to open another VST-plugin. After clicking on this button, a file-open-dialog appears and you can select your file.</source>
        <translation>Clic acá para abrir otro complemento VST. Luego aparecerá una ventana de diálogo para elegir el archivo.</translation>
    </message>
    <message>
        <source>Show/hide GUI</source>
        <translation>Mostrar/Ocultar interfaz gráfica (GUI)</translation>
    </message>
    <message>
        <source>Click here to show or hide the graphical user interface (GUI) of your VST-plugin.</source>
        <translation>Clic acá para mostrar u ocultar la interfaz gráfica (GUI) de tu complemento VST.</translation>
    </message>
    <message>
        <source>Turn off all notes</source>
        <translation>Apagar todas las notas</translation>
    </message>
    <message>
        <source>Open VST-plugin</source>
        <translation>Abrir complemento VST</translation>
    </message>
    <message>
        <source>DLL-files (*.dll)</source>
        <translation>archivos DLL (*.dll)</translation>
    </message>
    <message>
        <source>EXE-files (*.exe)</source>
        <translation>Archivos EXE (*.exe)</translation>
    </message>
    <message>
        <source>No VST-plugin loaded</source>
        <translation>No hay complementos VST cargados</translation>
    </message>
    <message>
        <source>Control VST-plugin from LMMS host</source>
        <translation>Controlar el complemento VST desde el anfitrión LMMS</translation>
    </message>
    <message>
        <source>Click here, if you want to control VST-plugin from host.</source>
        <translation>Clic acá si querés controlar el complemento VST-desde el anfitrión.</translation>
    </message>
    <message>
        <source>Open VST-plugin preset</source>
        <translation>Abrir preconfiguración de complemento VST</translation>
    </message>
    <message>
        <source>Click here, if you want to open another *.fxp, *.fxb VST-plugin preset.</source>
        <translation>Clic acá para abrir otro preset de complemento VST *.fxp, *.fxb.</translation>
    </message>
    <message>
        <source>Previous (-)</source>
        <translation>Anterior (-)</translation>
    </message>
    <message>
        <source>Click here, if you want to switch to another VST-plugin preset program.</source>
        <translation>Clic acá para cambiar a otro programa de configuración del complemento VST.</translation>
    </message>
    <message>
        <source>Save preset</source>
        <translation>Guardar configuración</translation>
    </message>
    <message>
        <source>Click here, if you want to save current VST-plugin preset program.</source>
        <translation>Clic acá para guardar la configuración actual del complemento VST.</translation>
    </message>
    <message>
        <source>Next (+)</source>
        <translation>Siguiente (+)</translation>
    </message>
    <message>
        <source>Click here to select presets that are currently loaded in VST.</source>
        <translation>Clic acá para seleccionar preconfiguraciones cargadas en VST.</translation>
    </message>
    <message>
        <source>Preset</source>
        <translation>Preconfiguración</translation>
    </message>
    <message>
        <source>by </source>
        <translation>por </translation>
    </message>
    <message>
        <source> - VST plugin control</source>
        <translation> - VST- control de complemento</translation>
    </message>
</context>
<context>
    <name>VstEffectControlDialog</name>
    <message>
        <source>Show/hide</source>
        <translation>Mostrar/Ocultar</translation>
    </message>
    <message>
        <source>Control VST-plugin from LMMS host</source>
        <translation>Controlar el complemento VST desde el anfitrión LMMS</translation>
    </message>
    <message>
        <source>Click here, if you want to control VST-plugin from host.</source>
        <translation>Clic acá si querés controlar el complemento VST-desde el anfitrión.</translation>
    </message>
    <message>
        <source>Open VST-plugin preset</source>
        <translation>Abrir preconfiguración de complemento VST</translation>
    </message>
    <message>
        <source>Click here, if you want to open another *.fxp, *.fxb VST-plugin preset.</source>
        <translation>Clic acá para abrir otro preset de complemento VST *.fxp, *.fxb.</translation>
    </message>
    <message>
        <source>Previous (-)</source>
        <translation>Anterior (-)</translation>
    </message>
    <message>
        <source>Click here, if you want to switch to another VST-plugin preset program.</source>
        <translation>Clic acá para cambiar a otro programa de configuración del complemento VST.</translation>
    </message>
    <message>
        <source>Next (+)</source>
        <translation>Siguiente (+)</translation>
    </message>
    <message>
        <source>Click here to select presets that are currently loaded in VST.</source>
        <translation>Clic acá para seleccionar preconfiguraciones cargadas en VST.</translation>
    </message>
    <message>
        <source>Save preset</source>
        <translation>Guardar configuración predeterminada</translation>
    </message>
    <message>
        <source>Click here, if you want to save current VST-plugin preset program.</source>
        <translation>Clic acá para guardar la configuración actual del complemento VST.</translation>
    </message>
    <message>
        <source>Effect by: </source>
        <translation>Efecto por: </translation>
    </message>
    <message>
        <source>&amp;nbsp;&amp;nbsp;&amp;nbsp;&amp;nbsp;&amp;nbsp;&amp;nbsp;&amp;nbsp;&lt;br /&gt;</source>
        <translation>&amp;nbsp;&amp;nbsp;&amp;nbsp;&amp;nbsp;&amp;nbsp;&amp;nbsp;&amp;nbsp;&lt;br /&gt;</translation>
    </message>
</context>
<context>
    <name>VstPlugin</name>
    <message>
        <source>Loading plugin</source>
        <translation>Cargando complemento</translation>
    </message>
    <message>
        <source>Please wait while loading VST-plugin...</source>
        <translation>Por favor espere, cargando complemento VST...</translation>
    </message>
    <message>
        <source>Failed loading VST-plugin</source>
        <translation>Error al cargar el complemento VST</translation>
    </message>
    <message>
        <source>The VST-plugin %1 could not be loaded for some reason.
If it runs with other VST-software under Linux, please contact an LMMS-developer!</source>
        <translation>El complemento VST %1 no pudo ser cargado por alguna razón. Si esto ocurre con otros complementos VST bajo gnu/linux, por favor contacte a un desarrollador de LMMS!</translation>
    </message>
    <message>
        <source>Open Preset</source>
        <translation>Abrir configuración</translation>
    </message>
    <message>
        <source>Vst Plugin Preset (*.fxp *.fxb)</source>
        <translation>Archivos de configuración VST (*.fxp, *.fxb)</translation>
    </message>
    <message>
        <source>: default</source>
        <translation>: defecto</translation>
    </message>
    <message>
        <source>&quot;</source>
        <translation>&quot;</translation>
    </message>
    <message>
        <source>&apos;</source>
        <translation>&apos;</translation>
    </message>
    <message>
        <source>Save Preset</source>
        <translation>Guardar configuración</translation>
    </message>
    <message>
        <source>.fxp</source>
        <translation>.fxb</translation>
    </message>
    <message>
        <source>.FXP</source>
        <translation>.FXB</translation>
    </message>
    <message>
        <source>.FXB</source>
        <translation>.FXB</translation>
    </message>
    <message>
        <source>.fxb</source>
        <translation>.fxb</translation>
    </message>
</context>
<context>
    <name>ZynAddSubFxInstrument</name>
    <message>
        <source>Portamento</source>
        <translation>Portamento</translation>
    </message>
    <message>
        <source>Filter Frequency</source>
        <translation>Filtro de frecuencia</translation>
    </message>
    <message>
        <source>Filter Resonance</source>
        <translation>Filtro de resonancia</translation>
    </message>
    <message>
        <source>Bandwidth</source>
        <translation>Ancho de banda</translation>
    </message>
    <message>
        <source>FM Gain</source>
        <translation>Ganancia FM</translation>
    </message>
    <message>
        <source>Resonance Center Frequency</source>
        <translation>Frecuencia central de resonancia</translation>
    </message>
    <message>
        <source>Resonance Bandwidth</source>
        <translation>Ancho de banda de resonancia</translation>
    </message>
    <message>
        <source>Forward MIDI Control Change Events</source>
        <translation>Adelantar cambio de control de eventos MIDI</translation>
    </message>
</context>
<context>
    <name>ZynAddSubFxView</name>
    <message>
        <source>Show GUI</source>
        <translation>Mostrar interfaz gráfica</translation>
    </message>
    <message>
        <source>Click here to show or hide the graphical user interface (GUI) of ZynAddSubFX.</source>
        <translation>Clic acá para mostrar u ocultar la interfaz gráfica de ZynAddSubFX.</translation>
    </message>
    <message>
        <source>Portamento:</source>
        <translation>Portamento:</translation>
    </message>
    <message>
        <source>PORT</source>
        <translation>PORT</translation>
    </message>
    <message>
        <source>Filter Frequency:</source>
        <translation>Filtro de frecuencia:</translation>
    </message>
    <message>
        <source>FREQ</source>
        <translation>FREC</translation>
    </message>
    <message>
        <source>Filter Resonance:</source>
        <translation>Filtro de resonancia:</translation>
    </message>
    <message>
        <source>RES</source>
        <translation>RES</translation>
    </message>
    <message>
        <source>Bandwidth:</source>
        <translation>Ancho de banda:</translation>
    </message>
    <message>
        <source>BW</source>
        <translation>AB</translation>
    </message>
    <message>
        <source>FM Gain:</source>
        <translation>Ganancia FM:</translation>
    </message>
    <message>
        <source>FM GAIN</source>
        <translation>GANANCIA FM</translation>
    </message>
    <message>
        <source>Resonance center frequency:</source>
        <translation>Frecuencia central de resonancia:</translation>
    </message>
    <message>
        <source>RES CF</source>
        <translation>RES CF</translation>
    </message>
    <message>
        <source>Resonance bandwidth:</source>
        <translation>Ancho de banda de resonancia:</translation>
    </message>
    <message>
        <source>RES BW</source>
        <translation>RES AB</translation>
    </message>
    <message>
        <source>Forward MIDI Control Changes</source>
        <translation>Adelantar cambios de control MIDI</translation>
    </message>
</context>
<context>
    <name>audioFileProcessor</name>
    <message>
        <source>Amplify</source>
        <translation>Amplificar</translation>
    </message>
    <message>
        <source>Start of sample</source>
        <translation>Inicio de la muestra</translation>
    </message>
    <message>
        <source>End of sample</source>
        <translation>Fin de la muestra</translation>
    </message>
    <message>
        <source>Reverse sample</source>
        <translation>Muestra en reversa</translation>
    </message>
    <message>
        <source>Loop</source>
        <translation>Bucle</translation>
    </message>
    <message>
        <source>Stutter</source>
        <translation></translation>
    </message>
</context>
<context>
    <name>bassBoosterControlDialog</name>
    <message>
        <source>FREQ</source>
        <translation>FREC</translation>
    </message>
    <message>
        <source>Frequency:</source>
        <translation>Frecuencia:</translation>
    </message>
    <message>
        <source>GAIN</source>
        <translation>Ganancia</translation>
    </message>
    <message>
        <source>Gain:</source>
        <translation>Ganancia:</translation>
    </message>
    <message>
        <source>RATIO</source>
        <translation>Ratio</translation>
    </message>
    <message>
        <source>Ratio:</source>
        <translation>Ratio:</translation>
    </message>
</context>
<context>
    <name>bassBoosterControls</name>
    <message>
        <source>Frequency</source>
        <translation>Frecuencia</translation>
    </message>
    <message>
        <source>Gain</source>
        <translation>Ganancia</translation>
    </message>
    <message>
        <source>Ratio</source>
        <translation>Ratio</translation>
    </message>
</context>
<context>
    <name>bbEditor</name>
    <message>
        <source>Play/pause current beat/bassline (Space)</source>
        <translation>Reproducir/pausar el ritmo base actual (espacio)</translation>
    </message>
    <message>
        <source>Beat+Bassline Editor</source>
        <translation>Editor Ritmo+Línea base</translation>
    </message>
    <message>
        <source>Add beat/bassline</source>
        <translation>Agregar Ritmo/Línea Base</translation>
    </message>
    <message>
        <source>Add automation-track</source>
        <translation>Agregar pista de Automatización</translation>
    </message>
    <message>
        <source>Stop playback of current beat/bassline (Space)</source>
        <translation>Detener el ritmo base actual (espacio)</translation>
    </message>
    <message>
        <source>Click here to play the current beat/bassline.  The beat/bassline is automatically looped when its end is reached.</source>
        <translation>Clic acá para reproducir el Ritmo-base actual. El ritmo-base se repite automáticamente cuando se llega al final.</translation>
    </message>
    <message>
        <source>Click here to stop playing of current beat/bassline.</source>
        <translation>Clic acá para Detener el ritmo base actual.</translation>
    </message>
    <message>
        <source>Remove steps</source>
        <translation>Eliminar pasos</translation>
    </message>
    <message>
        <source>Add steps</source>
        <translation>Agregar pasos</translation>
    </message>
</context>
<context>
    <name>bbTCOView</name>
    <message>
        <source>Open in Beat+Bassline-Editor</source>
        <translation>Abrir en Editor de Ritmo Base</translation>
    </message>
    <message>
        <source>Reset name</source>
        <translation>Reajustar nombre</translation>
    </message>
    <message>
        <source>Change name</source>
        <translation>Cambiar nombre</translation>
    </message>
    <message>
        <source>Change color</source>
        <translation>Cambiar color</translation>
    </message>
</context>
<context>
    <name>bbTrack</name>
    <message>
        <source>Beat/Bassline %1</source>
        <translation>Ritmo base %1</translation>
    </message>
    <message>
        <source>Clone of %1</source>
        <translation>Copia de %1</translation>
    </message>
</context>
<context>
    <name>bitInvader</name>
    <message>
        <source>Samplelength</source>
        <translation>Longitud de Muestra</translation>
    </message>
</context>
<context>
    <name>bitInvaderView</name>
    <message>
        <source>Sample Length</source>
        <translation>Duración de muestra</translation>
    </message>
    <message>
        <source>Sine wave</source>
        <translation>onda-senoidal</translation>
    </message>
    <message>
        <source>Triangle wave</source>
        <translation>onda-triangular</translation>
    </message>
    <message>
        <source>Saw wave</source>
        <translation>onda-sierra</translation>
    </message>
    <message>
        <source>Square wave</source>
        <translation>onda-cuadrada</translation>
    </message>
    <message>
        <source>White noise wave</source>
        <translation>onda-ruido-blanco</translation>
    </message>
    <message>
        <source>User defined wave</source>
        <translation>onda-definida por el usuario</translation>
    </message>
    <message>
        <source>Smooth</source>
        <translation>Suave</translation>
    </message>
    <message>
        <source>Click here to smooth waveform.</source>
        <translation>Clic acá para una onda cuadrada.</translation>
    </message>
    <message>
        <source>Interpolation</source>
        <translation>Interpolación</translation>
    </message>
    <message>
        <source>Normalize</source>
        <translation>Normalizar</translation>
    </message>
    <message>
        <source>Draw your own waveform here by dragging your mouse on this graph.</source>
        <translation>Dibuja con el ratón tu propia forma de onda en este gráfico.</translation>
    </message>
    <message>
        <source>Click for a sine-wave.</source>
        <translation>Clic acá para una onda sinusoidal.</translation>
    </message>
    <message>
        <source>Click here for a triangle-wave.</source>
        <translation>Clic acá para una onda triangular.</translation>
    </message>
    <message>
        <source>Click here for a saw-wave.</source>
        <translation>Clic acá para obtener una onda de sierra.</translation>
    </message>
    <message>
        <source>Click here for a square-wave.</source>
        <translation>Clic acá para una onda cuadrada.</translation>
    </message>
    <message>
        <source>Click here for white-noise.</source>
        <translation>Clic acá para ruido blanco.</translation>
    </message>
    <message>
        <source>Click here for a user-defined shape.</source>
        <translation>Clic acá para obtener una onda definida por vos.</translation>
    </message>
</context>
<context>
    <name>exportProjectDialog</name>
    <message>
        <source>Could not open file</source>
        <translation>No se puede abrir el archivo</translation>
    </message>
    <message>
        <source>Could not open file %1 for writing.
Please make sure you have write-permission to the file and the directory containing the file and try again!</source>
        <translation>No se puede abrir el archivo %1 para escritura.
Por favor asegurate de que tenés permiso de escritura sobre el archivo y sobre la carpeta que contiene al archivo e ¡intentá nuevamente!</translation>
    </message>
    <message>
        <source>Error</source>
        <translation>Error</translation>
    </message>
    <message>
        <source>Error while determining file-encoder device. Please try to choose a different output format.</source>
        <translation>Error al determinar dispositivo codificador del archivo. Por favor intenté seleccionar un formato de salida diferente.</translation>
    </message>
    <message>
        <source>Rendering: %1%</source>
        <translation>Exportando: %1%</translation>
    </message>
    <message>
        <source>Export project to %1</source>
        <translation>Exportar proyecto a %1</translation>
    </message>
</context>
<context>
    <name>fader</name>
    <message>
        <source>Please enter a new value between %1 and %2:</source>
        <translation>Por favor ingrese un nuevo valor entre %1 y %2:</translation>
    </message>
</context>
<context>
    <name>fileBrowser</name>
    <message>
        <source>Browser</source>
        <translation>Navegador</translation>
    </message>
</context>
<context>
    <name>fileBrowserTreeWidget</name>
    <message>
        <source>Send to active instrument-track</source>
        <translation>Enviar a pista de instrumento activa</translation>
    </message>
    <message>
        <source>Open in new instrument-track/Song-Editor</source>
        <translation>Abrir en una nueva pista de instrumento / Editor de Canción</translation>
    </message>
    <message>
        <source>Open in new instrument-track/B+B Editor</source>
        <translation>Abrir en una nueva pista de instrumento / Editor Ritmo-Línea Base </translation>
    </message>
    <message>
        <source>Loading sample</source>
        <translation>Cargando muestra</translation>
    </message>
    <message>
        <source>Please wait, loading sample for preview...</source>
        <translation>Por favor espere, cargando mustra para previsualización...</translation>
    </message>
    <message>
        <source>--- Factory files ---</source>
        <translation>---Archivos de fábrica---</translation>
    </message>
</context>
<context>
    <name>graphModel</name>
    <message>
        <source>Graph</source>
        <translation>Gráfico</translation>
    </message>
</context>
<context>
    <name>kickerInstrument</name>
    <message>
        <source>Start frequency</source>
        <translation>Frecuencia de inicio</translation>
    </message>
    <message>
        <source>End frequency</source>
        <translation>Frecuencia final</translation>
    </message>
    <message>
        <source>Decay</source>
        <translation>Decay</translation>
    </message>
    <message>
        <source>Distortion</source>
        <translation>Distorción</translation>
    </message>
    <message>
        <source>Gain</source>
        <translation>Ganancia</translation>
    </message>
</context>
<context>
    <name>kickerInstrumentView</name>
    <message>
        <source>Start frequency:</source>
        <translation>Frecuencia de inicio:</translation>
    </message>
    <message>
        <source>End frequency:</source>
        <translation>Frecuencia final:</translation>
    </message>
    <message>
        <source>Decay:</source>
        <translation>Decay:</translation>
    </message>
    <message>
        <source>Distortion:</source>
        <translation>Distorción:</translation>
    </message>
    <message>
        <source>Gain:</source>
        <translation>Ganancia:</translation>
    </message>
</context>
<context>
    <name>knob</name>
    <message>
        <source>&amp;Help</source>
        <translation>&amp;Ayuda</translation>
    </message>
    <message>
        <source>Please enter a new value between -96.0 dBV and 6.0 dBV:</source>
        <translation>Por favor ingrese un nuevo valor entre -96.0 dBV y 6.0 dBV:</translation>
    </message>
    <message>
        <source>Please enter a new value between %1 and %2:</source>
        <translation>Por favor ingrese un nuevo valor entre %1 y %2:</translation>
    </message>
</context>
<context>
    <name>ladspaBrowserView</name>
    <message>
        <source>Available Effects</source>
        <translation>Efectos disponibles</translation>
    </message>
    <message>
        <source>Unavailable Effects</source>
        <translation>Efectos no disponibles</translation>
    </message>
    <message>
        <source>Instruments</source>
        <translation>Instrumentos</translation>
    </message>
    <message>
        <source>Analysis Tools</source>
        <translation>Herramientas de análisis</translation>
    </message>
    <message>
        <source>Don&apos;t know</source>
        <translation>No lo sé</translation>
    </message>
    <message>
        <source>This dialog displays information on all of the LADSPA plugins LMMS was able to locate. The plugins are divided into five categories based upon an interpretation of the port types and names.

Available Effects are those that can be used by LMMS. In order for LMMS to be able to use an effect, it must, first and foremost, be an effect, which is to say, it has to have both input channels and output channels. LMMS identifies an input channel as an audio rate port containing &apos;in&apos; in the name. Output channels are identified by the letters &apos;out&apos;. Furthermore, the effect must have the same number of inputs and outputs and be real time capable.

Unavailable Effects are those that were identified as effects, but either didn&apos;t have the same number of input and output channels or weren&apos;t real time capable.

Instruments are plugins for which only output channels were identified.

Analysis Tools are plugins for which only input channels were identified.

Don&apos;t Knows are plugins for which no input or output channels were identified.

Double clicking any of the plugins will bring up information on the ports.</source>
        <translation>Este diálogo muestra información  en todos los complementos LADSPA que LMMS haya podido localizar. Los complementos se dividen en cinco categorías basadas en una interpretación de los tipos de puertos y nombres.

Solo estarán disponibles los efectos que pueden ser utilizados por LMMS. Para que LMMS sea capaz de utilizar un efecto, debe, ante todo, ser un efecto, es decir, que tiene que tener canales de entrada y canales de salida. LMMS identifica un canal de entrada como un puerto de tasa de audio que contiene &apos;IN&apos; en el nombre. Los canales de salida se identifican por las letras &apos;OUT&apos;. Además, el efecto debe tener el mismo número de entradas y salidas y ser capaz de funcionar en tiempo real.

Efectos no disponibles  son aquellos que fueron identificados como efectos pero no tienen el mismo número de entradas / salidas o no son capaces de funcionar en tiempo real.

Los instrumentos son complementos para los que se identificó sólo canales de salida.

Herramientas de análisis son complementos para los que se identificó sólo canales de entrada.

Hacer doble clic sobre cualquiera de los complementos hará aparecer información sobre los puertos.</translation>
    </message>
    <message>
        <source>Type:</source>
        <translation>Tipo:</translation>
    </message>
</context>
<context>
    <name>ladspaDescription</name>
    <message>
        <source>Plugins</source>
        <translation>Complementos</translation>
    </message>
    <message>
        <source>Description</source>
        <translation>Descripción</translation>
    </message>
</context>
<context>
    <name>ladspaPortDialog</name>
    <message>
        <source>Ports</source>
        <translation>Puertos</translation>
    </message>
    <message>
        <source>Name</source>
        <translation>Nombre</translation>
    </message>
    <message>
        <source>Rate</source>
        <translation>Ratio</translation>
    </message>
    <message>
        <source>Direction</source>
        <translation>Dirección</translation>
    </message>
    <message>
        <source>Type</source>
        <translation>Tipo</translation>
    </message>
    <message>
        <source>Min &lt; Default &lt; Max</source>
        <translation>Min &lt; defecto &lt; Max</translation>
    </message>
    <message>
        <source>Logarithmic</source>
        <translation>Logarítmico</translation>
    </message>
    <message>
        <source>SR Dependent</source>
        <translation>SR dependiente</translation>
    </message>
    <message>
        <source>Audio</source>
        <translation>Audio</translation>
    </message>
    <message>
        <source>Control</source>
        <translation>Control</translation>
    </message>
    <message>
        <source>Input</source>
        <translation>Entrada</translation>
    </message>
    <message>
        <source>Output</source>
        <translation>Salida</translation>
    </message>
    <message>
        <source>Toggled</source>
        <translation>Alternado</translation>
    </message>
    <message>
        <source>Integer</source>
        <translation>Entero</translation>
    </message>
    <message>
        <source>Float</source>
        <translation>Flotante</translation>
    </message>
    <message>
        <source>Yes</source>
        <translation>Si</translation>
    </message>
</context>
<context>
    <name>lb302Synth</name>
    <message>
        <source>VCF Cutoff Frequency</source>
        <translation>VCF Frecuencia de corte</translation>
    </message>
    <message>
        <source>VCF Resonance</source>
        <translation>Resonancia VCF</translation>
    </message>
    <message>
        <source>VCF Envelope Mod</source>
        <translation>VCF Mod envolvente</translation>
    </message>
    <message>
        <source>VCF Envelope Decay</source>
        <translation>VCF corte de envolvente</translation>
    </message>
    <message>
        <source>Distortion</source>
        <translation>Distorción</translation>
    </message>
    <message>
        <source>Waveform</source>
        <translation>Forma de onda</translation>
    </message>
    <message>
        <source>Slide Decay</source>
        <translation>Decaimiento de deslizamiento</translation>
    </message>
    <message>
        <source>Slide</source>
        <translation>Deslizamiento</translation>
    </message>
    <message>
        <source>Accent</source>
        <translation>Acentuar</translation>
    </message>
    <message>
        <source>Dead</source>
        <translation>Muerto</translation>
    </message>
    <message>
        <source>24dB/oct Filter</source>
        <translation>Filtro 24dB/oct</translation>
    </message>
</context>
<context>
    <name>lb302SynthView</name>
    <message>
        <source>Cutoff Freq:</source>
        <translation>Corte de Frecuencia:</translation>
    </message>
    <message>
        <source>Resonance:</source>
        <translation>Resonancia:</translation>
    </message>
    <message>
        <source>Env Mod:</source>
        <translation>Mod env:</translation>
    </message>
    <message>
        <source>Decay:</source>
        <translation>Decaimiento:</translation>
    </message>
    <message>
        <source>303-es-que, 24dB/octave, 3 pole filter</source>
        <translation>303-es-que, 24dB/octave, 3 pole filter</translation>
    </message>
    <message>
        <source>Slide Decay:</source>
        <translation>Decaimiento de deslizamiento:</translation>
    </message>
    <message>
        <source>DIST:</source>
        <translation>DIST:</translation>
    </message>
    <message>
        <source>Saw wave</source>
        <translation>onda-sierra</translation>
    </message>
    <message>
        <source>Click here for a saw-wave.</source>
        <translation>Clic acá para obtener una onda de sierra.</translation>
    </message>
    <message>
        <source>Triangle wave</source>
        <translation>onda-triangular</translation>
    </message>
    <message>
        <source>Click here for a triangle-wave.</source>
        <translation>Clic acá para una onda triangular.</translation>
    </message>
    <message>
        <source>Square wave</source>
        <translation>onda-cuadrada</translation>
    </message>
    <message>
        <source>Click here for a square-wave.</source>
        <translation>Clic acá para una onda cuadrada.</translation>
    </message>
    <message>
        <source>Rounded square wave</source>
        <translation>onda-cuadrada redondeada</translation>
    </message>
    <message>
        <source>Click here for a square-wave with a rounded end.</source>
        <translation>Clic acá para una onda cuadrada con un extremo redondeado.</translation>
    </message>
    <message>
        <source>Moog wave</source>
        <translation>Onda moog</translation>
    </message>
    <message>
        <source>Click here for a moog-like wave.</source>
        <translation>Clic acá para obtener una onda de sierra Moog.</translation>
    </message>
    <message>
        <source>Sine wave</source>
        <translation>onda-senoidal</translation>
    </message>
    <message>
        <source>Click for a sine-wave.</source>
        <translation>Clic acá para una onda sinusoidal.</translation>
    </message>
    <message>
        <source>White noise wave</source>
        <translation>onda-ruido-blanco</translation>
    </message>
    <message>
        <source>Click here for an exponential wave.</source>
        <translation>Clic acá para una onda exponencial.</translation>
    </message>
    <message>
        <source>Click here for white-noise.</source>
        <translation>Clic acá para ruido blanco.</translation>
    </message>
</context>
<context>
    <name>lb303Synth</name>
    <message>
        <source>VCF Cutoff Frequency</source>
        <translation>VCF Frecuencia de corte</translation>
    </message>
    <message>
        <source>VCF Resonance</source>
        <translation>Resonancia VCF</translation>
    </message>
    <message>
        <source>VCF Envelope Mod</source>
        <translation>VCF Mod envolvente</translation>
    </message>
    <message>
        <source>VCF Envelope Decay</source>
        <translation>VCF corte de envolvente</translation>
    </message>
    <message>
        <source>Distortion</source>
        <translation>Distorción</translation>
    </message>
    <message>
        <source>Waveform</source>
        <translation>Forma de onda</translation>
    </message>
    <message>
        <source>Slide Decay</source>
        <translation>Decaimiento de deslizamiento</translation>
    </message>
    <message>
        <source>Slide</source>
        <translation>Deslizamiento</translation>
    </message>
    <message>
        <source>Accent</source>
        <translation>Acentuar</translation>
    </message>
    <message>
        <source>Dead</source>
        <translation>Muerto</translation>
    </message>
    <message>
        <source>24dB/oct Filter</source>
        <translation>Filtro 24dB/oct</translation>
    </message>
</context>
<context>
    <name>lb303SynthView</name>
    <message>
        <source>Cutoff Freq:</source>
        <translation>Frecuencia de corte:</translation>
    </message>
    <message>
        <source>CUT</source>
        <translation>CORTAR</translation>
    </message>
    <message>
        <source>Resonance:</source>
        <translation>Resonancia:</translation>
    </message>
    <message>
        <source>RES</source>
        <translation>RES</translation>
    </message>
    <message>
        <source>Env Mod:</source>
        <translation>Mod env:</translation>
    </message>
    <message>
        <source>ENV MOD</source>
        <translation>MOD ENV</translation>
    </message>
    <message>
        <source>Decay:</source>
        <translation>Decaimiento:</translation>
    </message>
    <message>
        <source>DEC</source>
        <translation>DEC</translation>
    </message>
    <message>
        <source>303-es-que, 24dB/octave, 3 pole filter</source>
        <translation>303-es-que, 24dB/octave, 3 pole filter</translation>
    </message>
    <message>
        <source>Slide Decay:</source>
        <translation>Decaimiento de deslizamiento:</translation>
    </message>
    <message>
        <source>SLIDE</source>
        <translation>Deslizamiento</translation>
    </message>
    <message>
        <source>DIST:</source>
        <translation>DIST:</translation>
    </message>
    <message>
        <source>DIST</source>
        <translation>DIST</translation>
    </message>
    <message>
        <source>WAVE:</source>
        <translation>ONDA:</translation>
    </message>
    <message>
        <source>WAVE</source>
        <translation>ONDA</translation>
    </message>
</context>
<context>
    <name>malletsInstrument</name>
    <message>
        <source>Hardness</source>
        <translation>Dureza</translation>
    </message>
    <message>
        <source>Position</source>
        <translation>Posición</translation>
    </message>
    <message>
        <source>Vibrato Gain</source>
        <translation>Ganancia de Vibrato</translation>
    </message>
    <message>
        <source>Vibrato Freq</source>
        <translation>Frec Vibrato</translation>
    </message>
    <message>
        <source>Stick Mix</source>
        <translation>Stick Mix</translation>
    </message>
    <message>
        <source>Modulator</source>
        <translation>Modulador</translation>
    </message>
    <message>
        <source>Crossfade</source>
        <translation>Fundido cruzado</translation>
    </message>
    <message>
        <source>LFO Speed</source>
        <translation>Velocidad LFO</translation>
    </message>
    <message>
        <source>LFO Depth</source>
        <translation>Profundidad LFO</translation>
    </message>
    <message>
        <source>ADSR</source>
        <translation>ADSR</translation>
    </message>
    <message>
        <source>Pressure</source>
        <translation>Presión</translation>
    </message>
    <message>
        <source>Motion</source>
        <translation>Movimiento</translation>
    </message>
    <message>
        <source>Speed</source>
        <translation>Velocidad</translation>
    </message>
    <message>
        <source>Bowed</source>
        <translation>Inclinado</translation>
    </message>
    <message>
        <source>Spread</source>
        <translation>Spread</translation>
    </message>
    <message>
        <source>Marimba</source>
        <translation>Marimba</translation>
    </message>
    <message>
        <source>Vibraphone</source>
        <translation>Vibráfono</translation>
    </message>
    <message>
        <source>Agogo</source>
        <translation>Agogo</translation>
    </message>
    <message>
        <source>Wood1</source>
        <translation>Madera1</translation>
    </message>
    <message>
        <source>Reso</source>
        <translation>Reso</translation>
    </message>
    <message>
        <source>Wood2</source>
        <translation>Madera2</translation>
    </message>
    <message>
        <source>Beats</source>
        <translation>Golpes</translation>
    </message>
    <message>
        <source>Two Fixed</source>
        <translation>Dos Fijo</translation>
    </message>
    <message>
        <source>Clump</source>
        <translation>Grupo</translation>
    </message>
    <message>
        <source>Tubular Bells</source>
        <translation>Campanas tubulares</translation>
    </message>
    <message>
        <source>Uniform Bar</source>
        <translation>Barra uniforme</translation>
    </message>
    <message>
        <source>Tuned Bar</source>
        <translation>Tuned Bar</translation>
    </message>
    <message>
        <source>Glass</source>
        <translation>Vidrio</translation>
    </message>
    <message>
        <source>Tibetan Bowl</source>
        <translation>Cuenco Tibetano</translation>
    </message>
    <message>
        <source>Missing files</source>
        <translation>Archivos perdidos</translation>
    </message>
    <message>
        <source>Your Stk-installation seems to be incomplete. Please make sure the full Stk-package is installed!</source>
        <translation>Tu instalación STK parece estar incompleta. Por favor instalá el paquete STK completo!</translation>
    </message>
</context>
<context>
    <name>malletsInstrumentView</name>
    <message>
        <source>Instrument</source>
        <translation>Instrumento</translation>
    </message>
    <message>
        <source>Spread</source>
        <translation>Spread</translation>
    </message>
    <message>
        <source>Spread:</source>
        <translation>Spread:</translation>
    </message>
    <message>
        <source>Hardness</source>
        <translation>Dureza</translation>
    </message>
    <message>
        <source>Hardness:</source>
        <translation>Dureza:</translation>
    </message>
    <message>
        <source>Position</source>
        <translation>Posición</translation>
    </message>
    <message>
        <source>Position:</source>
        <translation>Posición:</translation>
    </message>
    <message>
        <source>Vib Gain</source>
        <translation>Ganancia Vib</translation>
    </message>
    <message>
        <source>Vib Gain:</source>
        <translation>Ganancia Vib:</translation>
    </message>
    <message>
        <source>Vib Freq</source>
        <translation>Frec Vibrato</translation>
    </message>
    <message>
        <source>Vib Freq:</source>
        <translation>Frec Vibrato:</translation>
    </message>
    <message>
        <source>Stick Mix</source>
        <translation>Stick Mix</translation>
    </message>
    <message>
        <source>Stick Mix:</source>
        <translation>Stick Mix:</translation>
    </message>
    <message>
        <source>Modulator</source>
        <translation>Modulador</translation>
    </message>
    <message>
        <source>Modulator:</source>
        <translation>Modulador:</translation>
    </message>
    <message>
        <source>Crossfade</source>
        <translation>Fundido cruzado</translation>
    </message>
    <message>
        <source>Crossfade:</source>
        <translation>Fundido cruzado:</translation>
    </message>
    <message>
        <source>LFO Speed</source>
        <translation>LFO velocidad</translation>
    </message>
    <message>
        <source>LFO Speed:</source>
        <translation>LFO velocidad:</translation>
    </message>
    <message>
        <source>LFO Depth</source>
        <translation>Profundidad LFO</translation>
    </message>
    <message>
        <source>LFO Depth:</source>
        <translation>Profundidad LFO:</translation>
    </message>
    <message>
        <source>ADSR</source>
        <translation>ADSR</translation>
    </message>
    <message>
        <source>ADSR:</source>
        <translation>ADSR:</translation>
    </message>
    <message>
        <source>Bowed</source>
        <translation>Inclinado</translation>
    </message>
    <message>
        <source>Pressure</source>
        <translation>Presión</translation>
    </message>
    <message>
        <source>Pressure:</source>
        <translation>Presión:</translation>
    </message>
    <message>
        <source>Motion</source>
        <translation>Movimiento</translation>
    </message>
    <message>
        <source>Motion:</source>
        <translation>Movimiento:</translation>
    </message>
    <message>
        <source>Speed</source>
        <translation>Velocidad</translation>
    </message>
    <message>
        <source>Speed:</source>
        <translation>Velocidad:</translation>
    </message>
    <message>
        <source>Vibrato</source>
        <translation>Vibrato</translation>
    </message>
    <message>
        <source>Vibrato:</source>
        <translation>Vibrato:</translation>
    </message>
</context>
<context>
    <name>manageVSTEffectView</name>
    <message>
        <source> - VST parameter control</source>
        <translation>control de parámetros - VST</translation>
    </message>
    <message>
        <source>VST Sync</source>
        <translation>VST sincro</translation>
    </message>
    <message>
        <source>Click here if you want to synchronize all parameters with VST plugin.</source>
        <translation>Clic acá para sincronizar todos los parámetros con el Complemento VST.</translation>
    </message>
    <message>
        <source>Automated</source>
        <translation>Automatizado</translation>
    </message>
    <message>
        <source>Click here if you want to display automated parameters only.</source>
        <translation>Clic acá para mostrar los parámetros de automatización únicamente.</translation>
    </message>
    <message>
        <source>    Close    </source>
        <translation>····Cerrar </translation>
    </message>
    <message>
        <source>Close VST effect knob-controller window.</source>
        <translation>Cerrar la ventana de control del efecto VST.</translation>
    </message>
</context>
<context>
    <name>manageVestigeInstrumentView</name>
    <message>
        <source> - VST plugin control</source>
        <translation> - VST- control de complemento</translation>
    </message>
    <message>
        <source>VST Sync</source>
        <translation>VST sincro</translation>
    </message>
    <message>
        <source>Click here if you want to synchronize all parameters with VST plugin.</source>
        <translation>Clic acá para sincronizar todos los parámetros con el Complemento VST.</translation>
    </message>
    <message>
        <source>Automated</source>
        <translation>Automatizado</translation>
    </message>
    <message>
        <source>Click here if you want to display automated parameters only.</source>
        <translation>Clic acá para mostrar los parámetros de automatización únicamente.</translation>
    </message>
    <message>
        <source>    Close    </source>
        <translation>····Cerrar </translation>
    </message>
    <message>
        <source>Close VST plugin knob-controller window.</source>
        <translation>Cerrar la ventana de control del efecto VST.</translation>
    </message>
</context>
<context>
    <name>nineButtonSelector</name>
    <message>
        <source>&amp;Help</source>
        <translation>&amp;Ayuda</translation>
    </message>
</context>
<context>
    <name>opl2instrument</name>
    <message>
        <source>Patch</source>
        <translation>Parche</translation>
    </message>
    <message>
        <source>Op 1 Attack</source>
        <translation>Op 1 Ataque</translation>
    </message>
    <message>
        <source>Op 1 Decay</source>
        <translation>Op 1 Decaimiento</translation>
    </message>
    <message>
        <source>Op 1 Sustain</source>
        <translation>Op 1 Sostenido</translation>
    </message>
    <message>
        <source>Op 1 Release</source>
        <translation>Op 1 Liberación</translation>
    </message>
    <message>
        <source>Op 1 Level</source>
        <translation>Op 1 Nivel</translation>
    </message>
    <message>
        <source>Op 1 Level Scaling</source>
        <translation>Op 1 Escala de Nivel</translation>
    </message>
    <message>
        <source>Op 1 Frequency Multiple</source>
        <translation>Op 1 Múltiplo de frecuencia</translation>
    </message>
    <message>
        <source>Op 1 Feedback</source>
        <translation>Op 1 Realimentación</translation>
    </message>
    <message>
        <source>Op 1 Key Scaling Rate</source>
        <translation>Op 1 Ratio de clave de escala </translation>
    </message>
    <message>
        <source>Op 1 Percussive Envelope</source>
        <translation>Op 1 envolvente de percución</translation>
    </message>
    <message>
        <source>Op 1 Tremolo</source>
        <translation>Op 1 Trémolo</translation>
    </message>
    <message>
        <source>Op 1 Vibrato</source>
        <translation>Op 1 Vibrato</translation>
    </message>
    <message>
        <source>Op 1 Waveform</source>
        <translation>Op 1 Forma de onda</translation>
    </message>
    <message>
        <source>Op 2 Attack</source>
        <translation>Op 2 Ataque</translation>
    </message>
    <message>
        <source>Op 2 Decay</source>
        <translation>Op 2 Decaimiento</translation>
    </message>
    <message>
        <source>Op 2 Sustain</source>
        <translation>Op 2 Sostenido</translation>
    </message>
    <message>
        <source>Op 2 Release</source>
        <translation>Op 2 Liberación</translation>
    </message>
    <message>
        <source>Op 2 Level</source>
        <translation>Op 2 Nivel</translation>
    </message>
    <message>
        <source>Op 2 Level Scaling</source>
        <translation>Op 2 Escala de Nivel</translation>
    </message>
    <message>
        <source>Op 2 Frequency Multiple</source>
        <translation>Op 2 Múltiplo de frecuencia</translation>
    </message>
    <message>
        <source>Op 2 Key Scaling Rate</source>
        <translation>Op 2 Ratio de clave de escala </translation>
    </message>
    <message>
        <source>Op 2 Percussive Envelope</source>
        <translation>Op 2 envolvente de percución</translation>
    </message>
    <message>
        <source>Op 2 Tremolo</source>
        <translation>Op 2 Trémolo</translation>
    </message>
    <message>
        <source>Op 2 Vibrato</source>
        <translation>Op 2 Vibrato</translation>
    </message>
    <message>
        <source>Op 2 Waveform</source>
        <translation>Op 2 Forma de onda</translation>
    </message>
    <message>
        <source>FM</source>
        <translation>FM</translation>
    </message>
    <message>
        <source>Vibrato Depth</source>
        <translation>Profundidad del Vibrato</translation>
    </message>
    <message>
        <source>Tremolo Depth</source>
        <translation>Profundidad del Trémolo</translation>
    </message>
</context>
<context>
    <name>organicInstrument</name>
    <message>
        <source>Distortion</source>
        <translation>Distorción</translation>
    </message>
    <message>
        <source>Volume</source>
        <translation>Volumen</translation>
    </message>
</context>
<context>
    <name>organicInstrumentView</name>
    <message>
        <source>Distortion:</source>
        <translation>Distorción:</translation>
    </message>
    <message>
        <source>Volume:</source>
        <translation>Volumen:</translation>
    </message>
    <message>
        <source>Randomise</source>
        <translation>Manera Aleatoria</translation>
    </message>
    <message>
        <source>Osc %1 waveform:</source>
        <translation>OSC %1 Forma de onda:</translation>
    </message>
    <message>
        <source>Osc %1 volume:</source>
        <translation>Osc %1 Volumen:</translation>
    </message>
    <message>
        <source>Osc %1 panning:</source>
        <translation>Osc %1 encuadramiento:</translation>
    </message>
    <message>
        <source>Osc %1 fine detuning left:</source>
        <translation>Osc %1 desintonización fina izquierda:</translation>
    </message>
    <message>
        <source>cents</source>
        <translation>cents</translation>
    </message>
</context>
<context>
    <name>papuInstrument</name>
    <message>
        <source>Sweep time</source>
        <translation>Tiempo de barrido</translation>
    </message>
    <message>
        <source>Sweep direction</source>
        <translation>Dirección de barrida</translation>
    </message>
    <message>
        <source>Sweep RtShift amount</source>
        <translation>Cantidad de barrido RtShift</translation>
    </message>
    <message>
        <source>Wave Pattern Duty</source>
        <translation>Deber de patrón de onda</translation>
    </message>
    <message>
        <source>Channel 1 volume</source>
        <translation>Volumen del canal 1</translation>
    </message>
    <message>
        <source>Volume sweep direction</source>
        <translation>Dirección de barrido de volumen</translation>
    </message>
    <message>
        <source>Length of each step in sweep</source>
        <translation>Largo de cada paso en barrida</translation>
    </message>
    <message>
        <source>Channel 2 volume</source>
        <translation>Volumen del canal 2</translation>
    </message>
    <message>
        <source>Channel 3 volume</source>
        <translation>Volumen del canal 3</translation>
    </message>
    <message>
        <source>Channel 4 volume</source>
        <translation>Volumen del canal 4</translation>
    </message>
    <message>
        <source>Right Output level</source>
        <translation>Nivel de Salida Derecho</translation>
    </message>
    <message>
        <source>Left Output level</source>
        <translation>Nivel de Salida Izquierdo</translation>
    </message>
    <message>
        <source>Channel 1 to SO2 (Left)</source>
        <translation>Canal 1 a SO2 (Izquierda)</translation>
    </message>
    <message>
        <source>Channel 2 to SO2 (Left)</source>
        <translation>Canal 2 a SO2 (Izquierda)</translation>
    </message>
    <message>
        <source>Channel 3 to SO2 (Left)</source>
        <translation>Canal 3 a SO2 (Izquierda)</translation>
    </message>
    <message>
        <source>Channel 4 to SO2 (Left)</source>
        <translation>Canal 4 a SO2 (Izquierda)</translation>
    </message>
    <message>
        <source>Channel 1 to SO1 (Right)</source>
        <translation>Canal 1 a SO1 (Derecha)</translation>
    </message>
    <message>
        <source>Channel 2 to SO1 (Right)</source>
        <translation>Canal 2 a SO1 (Derecha)</translation>
    </message>
    <message>
        <source>Channel 3 to SO1 (Right)</source>
        <translation>Canal 3 a SO1 (Derecha)</translation>
    </message>
    <message>
        <source>Channel 4 to SO1 (Right)</source>
        <translation>Canal 4 a SO1 (Derecha)</translation>
    </message>
    <message>
        <source>Treble</source>
        <translation>Agudos</translation>
    </message>
    <message>
        <source>Bass</source>
        <translation>Bajo</translation>
    </message>
    <message>
        <source>Shift Register width</source>
        <translation>Ancho del Registro de desplazamiento</translation>
    </message>
</context>
<context>
    <name>papuInstrumentView</name>
    <message>
        <source>Sweep Time:</source>
        <translation>Tiempo de barrido:</translation>
    </message>
    <message>
        <source>Sweep Time</source>
        <translation>Tiempo de barrido</translation>
    </message>
    <message>
        <source>Sweep RtShift amount:</source>
        <translation>Cantidad de barrido RtShift:</translation>
    </message>
    <message>
        <source>Sweep RtShift amount</source>
        <translation>Cantidad de barrido RtShift</translation>
    </message>
    <message>
        <source>Wave pattern duty:</source>
        <translation>Deber de patrón de onda:</translation>
    </message>
    <message>
        <source>Wave Pattern Duty</source>
        <translation>Deber de patrón de onda</translation>
    </message>
    <message>
        <source>Square Channel 1 Volume:</source>
        <translation>Volumen Square Channel 1:</translation>
    </message>
    <message>
        <source>Length of each step in sweep:</source>
        <translation>Longitud de cada paso en el barrido:</translation>
    </message>
    <message>
        <source>Length of each step in sweep</source>
        <translation>Longitud de cada paso en el barrido</translation>
    </message>
    <message>
        <source>Wave pattern duty</source>
        <translation>Deber de patrón de onda</translation>
    </message>
    <message>
        <source>Square Channel 2 Volume:</source>
        <translation>Volumen Square Channel 2:</translation>
    </message>
    <message>
        <source>Square Channel 2 Volume</source>
        <translation>Volumen Square Channel 2</translation>
    </message>
    <message>
        <source>Wave Channel Volume:</source>
        <translation>Volumen de canal de onda:</translation>
    </message>
    <message>
        <source>Wave Channel Volume</source>
        <translation>Volumen de canal de onda</translation>
    </message>
    <message>
        <source>Noise Channel Volume:</source>
        <translation>Volumen de Ruido en Canal:</translation>
    </message>
    <message>
        <source>Noise Channel Volume</source>
        <translation>Volumen de Ruido en Canal</translation>
    </message>
    <message>
        <source>SO1 Volume (Right):</source>
        <translation>Volumen SO1 (Derecho):</translation>
    </message>
    <message>
        <source>SO1 Volume (Right)</source>
        <translation>Volumen SO1 (Derecho)</translation>
    </message>
    <message>
        <source>SO2 Volume (Left):</source>
        <translation>Volumen SO2 (Izquierda):</translation>
    </message>
    <message>
        <source>SO2 Volume (Left)</source>
        <translation>Volumen SO2 (Izquierda)</translation>
    </message>
    <message>
        <source>Treble:</source>
        <translation>Agudos:</translation>
    </message>
    <message>
        <source>Treble</source>
        <translation>Agudos</translation>
    </message>
    <message>
        <source>Bass:</source>
        <translation>Bajo:</translation>
    </message>
    <message>
        <source>Bass</source>
        <translation>Bajo</translation>
    </message>
    <message>
        <source>Sweep Direction</source>
        <translation>Dirección de barrido</translation>
    </message>
    <message>
        <source>Volume Sweep Direction</source>
        <translation>Dirección de barrido de volumen</translation>
    </message>
    <message>
        <source>Shift Register Width</source>
        <translation>Ancho de registro de desplazamiento</translation>
    </message>
    <message>
        <source>Channel1 to SO1 (Right)</source>
        <translation>Canal1 a SO1 (Derecha)</translation>
    </message>
    <message>
        <source>Channel2 to SO1 (Right)</source>
        <translation>Canal2 a SO1 (Derecha)</translation>
    </message>
    <message>
        <source>Channel3 to SO1 (Right)</source>
        <translation>Canal3 a SO1 (Derecha)</translation>
    </message>
    <message>
        <source>Channel4 to SO1 (Right)</source>
        <translation>Canal4 a SO1 (Derecha)</translation>
    </message>
    <message>
        <source>Channel1 to SO2 (Left)</source>
        <translation>Canal1 a SO2 (Izquierda)</translation>
    </message>
    <message>
        <source>Channel2 to SO2 (Left)</source>
        <translation>Canal2 a SO2 (Izquierda)</translation>
    </message>
    <message>
        <source>Channel3 to SO2 (Left)</source>
        <translation>Canal3 a SO2 (Izquierda)</translation>
    </message>
    <message>
        <source>Channel4 to SO2 (Left)</source>
        <translation>Canal4 a SO2 (Izquierda)</translation>
    </message>
    <message>
        <source>Wave Pattern</source>
        <translation>Patrón de onda</translation>
    </message>
    <message>
        <source>The amount of increase or decrease in frequency</source>
        <translation>La cantidad de aumento o disminución en la frecuencia</translation>
    </message>
    <message>
        <source>The rate at which increase or decrease in frequency occurs</source>
        <translation>La velocidad a la que ocurre el aumento o disminución de frecuencia</translation>
    </message>
    <message>
        <source>The duty cycle is the ratio of the duration (time) that a signal is ON versus the total period of the signal.</source>
        <translation>El ciclo de trabajo es la relación de la duración (tiempo) que una señal está en ON en comparación con el período total de la señal.</translation>
    </message>
    <message>
        <source>Square Channel 1 Volume</source>
        <translation>Volumen Square Channel 1</translation>
    </message>
    <message>
        <source>The delay between step change</source>
        <translation>El retraso entre cambios de paso</translation>
    </message>
    <message>
        <source>Draw the wave here</source>
        <translation>Dibujar la onda acá</translation>
    </message>
</context>
<context>
    <name>pattern</name>
    <message>
        <source>Cannot freeze pattern</source>
        <translation>No se puede congelar el patrón</translation>
    </message>
    <message>
        <source>The pattern currently cannot be freezed because you&apos;re in play-mode. Please stop and try again!</source>
        <translation>El patrón actual no puede ser congelado debido a que usted esta en el modo de reproducción. Por favor detenga la reproducción y vuelva a intentarlo!</translation>
    </message>
</context>
<context>
    <name>patternFreezeStatusDialog</name>
    <message>
        <source>Freezing pattern...</source>
        <translation>Congelando patrón...</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation>Cancelar</translation>
    </message>
</context>
<context>
    <name>patternView</name>
    <message>
        <source>double-click to open this pattern in piano-roll
use mouse wheel to set volume of a step</source>
        <translation>Doble clic para abrir este patrón en el Piano-Roll. Con la rueda del ratón se puede modificar el volumen de un paso</translation>
    </message>
    <message>
        <source>Open in piano-roll</source>
        <translation>Abrir en piano-roll</translation>
    </message>
    <message>
        <source>Clear all notes</source>
        <translation>Borrar todas las notas</translation>
    </message>
    <message>
        <source>Reset name</source>
        <translation>Restablecer nombre</translation>
    </message>
    <message>
        <source>Change name</source>
        <translation>Cambiar nombre</translation>
    </message>
    <message>
        <source>Refreeze</source>
        <translation>Recongelar</translation>
    </message>
    <message>
        <source>Freeze</source>
        <translation>Congelar</translation>
    </message>
    <message>
        <source>Unfreeze</source>
        <translation>Descongelar</translation>
    </message>
    <message>
        <source>Add steps</source>
        <translation>Agregar pasos</translation>
    </message>
    <message>
        <source>Remove steps</source>
        <translation>Eliminar pasos</translation>
    </message>
</context>
<context>
    <name>PianoRoll</name>
    <message>
        <source>Cut selected notes (Ctrl+X)</source>
        <translation>Cortar las notas seleccionadas (Ctrl+X)</translation>
    </message>
    <message>
        <source>Copy selected notes (Ctrl+C)</source>
        <translation>Copiar las notas seleccionadas (Ctrl+C)</translation>
    </message>
    <message>
        <source>Paste notes from clipboard (Ctrl+V)</source>
        <translation>Pegar notas desde el portapapeles (Ctrl+V)</translation>
    </message>
    <message>
        <source>Play/pause current pattern (Space)</source>
        <translation>Reproducir/Pausar el patrón actual (Espaciador)</translation>
    </message>
    <message>
        <source>Stop playing of current pattern (Space)</source>
        <translation>Detener la reproducción del patrón actual (Espaciador)</translation>
    </message>
    <message>
        <source>Piano-Roll - no pattern</source>
        <translation>Piano Roll - ningún patrón</translation>
    </message>
    <message>
        <source>Please open a pattern by double-clicking on it!</source>
        <translation>Por favor abra el patrón haciendo doble click sobre él!</translation>
    </message>
    <message>
        <source>Piano-Roll - %1</source>
        <translation>Piano-Roll - %1</translation>
    </message>
    <message>
        <source>Record notes from MIDI-device/channel-piano</source>
        <translation>Grabar notas desde un dispositivo MIDI o canal Piano</translation>
    </message>
    <message>
        <source>Record notes from MIDI-device/channel-piano while playing song or BB track</source>
        <translation>Grabar notas desde un dispositivo MIDI o canal Piano mientras suena la canción</translation>
    </message>
    <message>
        <source>Draw mode (Shift+D)</source>
        <translation>Modo Dibujo (Shift+D)</translation>
    </message>
    <message>
        <source>Erase mode (Shift+E)</source>
        <translation>Modo Borrar (Shift+E)</translation>
    </message>
    <message>
        <source>Select mode (Shift+S)</source>
        <translation>Modo Selección (Shift+S)</translation>
    </message>
    <message>
        <source>Last note</source>
        <translation>Última nota</translation>
    </message>
    <message>
        <source>Click here to play the current pattern. This is useful while editing it. The pattern is automatically looped when its end is reached.</source>
        <translation>Haga clic aquí si desea reproducir el patrón actual. Esto es útil durante la edición del mismo. El patrón se repite automáticamente cuando se llega al final.</translation>
    </message>
    <message>
        <source>Click here to record notes from a MIDI-device or the virtual test-piano of the according channel-window to the current pattern. When recording all notes you play will be written to this pattern and you can play and edit them afterwards.</source>
        <translation>Clic acá para grabar notas desde un dispositivo MIDI o desde el teclado virtual  en el patrón actual. Al grabar, todas las notas que toque serán grabadas al patrón actual y luego podrá reproducirlas y editarlas.</translation>
    </message>
    <message>
        <source>Click here to record notes from a MIDI-device or the virtual test-piano of the according channel-window to the current pattern. When recording all notes you play will be written to this pattern and you will hear the song or BB track in the background.</source>
        <translation>Clic acá para grabar notas desde un dispositivo MIDI o desde el teclado virtual  en el patrón actual. Al grabar, todas las notas que toque serán grabadas al patrón actual y luego podrá reproducir la canción o Track BB en el fondo.</translation>
    </message>
    <message>
        <source>Click here to stop playback of current pattern.</source>
        <translation>Clic acá para detener la reproducción del patrón actual.</translation>
    </message>
    <message>
        <source>Click here and the selected notes will be cut into the clipboard. You can paste them anywhere in any pattern by clicking on the paste button.</source>
        <translation>Clic acá para cortar los valores seleccionados al portapapeles. Puedes pegarlos donde quieras en un patrón haciendo clic en el botón pegar.</translation>
    </message>
    <message>
        <source>Click here and the selected notes will be copied into the clipboard. You can paste them anywhere in any pattern by clicking on the paste button.</source>
        <translation>Clic acá para copiar los valores seleccionados al portapapeles. Puedes pegarlos donde quieras en un patrón haciendo clic en el botón pegar.</translation>
    </message>
    <message>
        <source>Click here and the notes from the clipboard will be pasted at the first visible measure.</source>
        <translation>Clic acá para pegar los valores del portapapeles en la primera medida visible.</translation>
    </message>
    <message>
        <source>Note lock</source>
        <translation>bloqueo de nota</translation>
    </message>
    <message>
        <source>Note Volume</source>
        <translation>Volumen de la nota</translation>
    </message>
    <message>
        <source>Note Panning</source>
        <translation>Paneo de Nota</translation>
    </message>
    <message>
        <source>Detune mode (Shift+T)</source>
        <translation>Modo Desafinación (Shift+T)</translation>
    </message>
    <message>
        <source>Click here and draw mode will be activated. In this mode you can add, resize and move notes. This is the default mode which is used most of the time. You can also press &apos;Shift+D&apos; on your keyboard to activate this mode. In this mode, hold Ctrl to temporarily go into select mode.</source>
        <translation>Clic acá para activar el modo dibujo. En este modo podés agregar, cambiar de tamaño y mover notas. Este es el modo por defecto que se utiliza la mayor parte del tiempo. También puede pulsar &apos;Shift + D&apos; en el teclado para activar este modo. Mantené presionado &lt;Ctrl&gt; para ir al modo seleción temporalmente.</translation>
    </message>
    <message>
        <source>Click here and erase mode will be activated. In this mode you can erase notes. You can also press &apos;Shift+E&apos; on your keyboard to activate this mode.</source>
        <translation>Clic acá para activar el modo Borrar. En este modo se pueden eliminar notas. También puede pulsar &apos;Shift + E&apos; en el teclado para activar este modo.</translation>
    </message>
    <message>
        <source>Click here and select mode will be activated. In this mode you can select notes. Alternatively, you can hold Ctrl in draw mode to temporarily use select mode.</source>
        <translation>Clic acá para activar el modo Selección. En este modo se pueden seleccionar notas. También podés mantener presionado &lt;Ctrl&gt; en el modo Dibujo para pasar a este modo temporalmente.</translation>
    </message>
    <message>
        <source>Click here and detune mode will be activated. In this mode you can click a note to open its automation detuning. You can utilize this to slide notes from one to another. You can also press &apos;Shift+T&apos; on your keyboard to activate this mode.</source>
        <translation>Clic acá para activar el modo de desafinación. En este modo, puede hacer clic en una nota para abrir su automatización de desafinación . Podés utilizar ésta para deslizar notas de una a otra. También podés pulsar &apos;Shift + T&apos; en el teclado para activar este modo.</translation>
    </message>
    <message>
        <source>Mark/unmark current semitone</source>
        <translation></translation>
    </message>
    <message>
        <source>Mark current scale</source>
        <translation>Marcar escala actual</translation>
    </message>
    <message>
        <source>Mark current chord</source>
        <translation>Marcar acorde actual</translation>
    </message>
    <message>
        <source>Unmark all</source>
        <translation>Desmarcar todo</translation>
    </message>
    <message>
        <source>No scale</source>
        <translation>Sin escala</translation>
    </message>
    <message>
        <source>No chord</source>
        <translation>Sin acorde</translation>
    </message>
</context>
<context>
    <name>pluginBrowser</name>
    <message>
        <source>no description</source>
        <translation>sin descripción</translation>
    </message>
    <message>
        <source>Instrument plugins</source>
        <translation>Instrumentos</translation>
    </message>
    <message>
        <source>Incomplete monophonic imitation tb303</source>
        <translation>Imitación monofónica incompleta tb303</translation>
    </message>
    <message>
        <source>Plugin for freely manipulating stereo output</source>
        <translation>Complemento para manipular libremente salida estéreo</translation>
    </message>
    <message>
        <source>Plugin for controlling knobs with sound peaks</source>
        <translation>Complemento para el control de mandos con picos de sonido</translation>
    </message>
    <message>
        <source>Plugin for enhancing stereo separation of a stereo input file</source>
        <translation>Complemento para mejorar la separación estéreo de un archivo de entrada estéreo</translation>
    </message>
    <message>
        <source>List installed LADSPA plugins</source>
        <translation>Lista de complementos LADSPA instalados</translation>
    </message>
    <message>
        <source>three powerful oscillators you can modulate in several ways</source>
        <translation>Tres osciladores de gran alcance que pueden modular de varias maneras</translation>
    </message>
    <message>
        <source>Filter for importing FL Studio projects into LMMS</source>
        <translation>Filtro para importar proyectos de FL Studio a LMMS</translation>
    </message>
    <message>
        <source>versatile kick- &amp; bassdrum-synthesizer</source>
        <translation>Sintetizador de Bombo versátil</translation>
    </message>
    <message>
        <source>GUS-compatible patch instrument</source>
        <translation>Instrumento parche compatible con GUS</translation>
    </message>
    <message>
        <source>plugin for using arbitrary VST-effects inside LMMS.</source>
        <translation>Complemento para usar arbitrariamente efectos VST dentro de LMMS.</translation>
    </message>
    <message>
        <source>Additive Synthesizer for organ-like sounds</source>
        <translation>Sintetizador aditivo para sonidos tipo-órgano</translation>
    </message>
    <message>
        <source>plugin for boosting bass</source>
        <translation>Complemento para impulsar el Bajo</translation>
    </message>
    <message>
        <source>Tuneful things to bang on</source>
        <translation>Cosas melodiosas que golpean</translation>
    </message>
    <message>
        <source>simple sampler with various settings for using samples (e.g. drums) in an instrument-track</source>
        <translation>sampler simple con varios ajustes para usar muestras (por ejemplo, batería) en una pista-instrumento</translation>
    </message>
    <message>
        <source>VST-host for using VST(i)-plugins within LMMS</source>
        <translation>Anfitrión VST para usar complementos VST dentro de LMMS</translation>
    </message>
    <message>
        <source>Vibrating string modeler</source>
        <translation>Modulador de Cuerdas vibrantes</translation>
    </message>
    <message>
        <source>plugin for using arbitrary LADSPA-effects inside LMMS.</source>
        <translation>Complemento para usar arbitrariamente efectos LADSPA dentro de LMMS.</translation>
    </message>
    <message>
        <source>Filter for importing MIDI-files into LMMS</source>
        <translation>Filtro para importar archivos MIDI a LMMS</translation>
    </message>
    <message>
        <source>Instrument browser</source>
        <translation>Navegador de Instrumentos</translation>
    </message>
    <message>
        <source>Drag an instrument into either the Song-Editor, the Beat+Bassline Editor or into an existing instrument track.</source>
        <translation>Arrastre un instrumento al editor de Canción, al editor Ritmo- Línea Base o a una pista de instrumento existente.</translation>
    </message>
    <message>
        <source>Emulation of the MOS6581 and MOS8580 SID.
This chip was used in the Commodore 64 computer.</source>
        <translation>Emulador del chip SID MOS6581 y MOS8580.
El chip de sonido de la Commodore 64.</translation>
    </message>
    <message>
        <source>Player for SoundFont files</source>
        <translation>Reproductor para archivos SoundFont</translation>
    </message>
    <message>
        <source>Emulation of GameBoy (TM) APU</source>
        <translation>Emulador de GameBoy (R) APU</translation>
    </message>
    <message>
        <source>Customizable wavetable synthesizer</source>
        <translation>Sintetizador de tabla de ondas Personalizable</translation>
    </message>
    <message>
        <source>Embedded ZynAddSubFX</source>
        <translation>ZynAddSubFX embebido</translation>
    </message>
    <message>
        <source>2-operator FM Synth</source>
        <translation>2-operator FM Synth</translation>
    </message>
    <message>
        <source>Filter for importing Hydrogen files into LMMS</source>
        <translation>Filtro para importar proyectos de Hydrogen a LMMS</translation>
    </message>
    <message>
        <source>LMMS port of sfxr</source>
        <translation>porte de sfxr para LMMS</translation>
    </message>
</context>
<context>
    <name>projectNotes</name>
    <message>
        <source>Put down your project notes here.</source>
        <translation>Coloque aquí sus notas del proyecto.</translation>
    </message>
    <message>
        <source>Project notes</source>
        <translation>Notas del Proyecto</translation>
    </message>
    <message>
        <source>Edit Actions</source>
        <translation>Editar Acciones</translation>
    </message>
    <message>
        <source>&amp;Undo</source>
        <translation>&amp;Deshacer</translation>
    </message>
    <message>
        <source>Ctrl+Z</source>
        <translation>Ctrl+Z</translation>
    </message>
    <message>
        <source>&amp;Redo</source>
        <translation>&amp;Rehacer</translation>
    </message>
    <message>
        <source>Ctrl+Y</source>
        <translation>Ctrl+Y</translation>
    </message>
    <message>
        <source>&amp;Copy</source>
        <translation>&amp;Copiar</translation>
    </message>
    <message>
        <source>Ctrl+C</source>
        <translation>Ctrl+C</translation>
    </message>
    <message>
        <source>Cu&amp;t</source>
        <translation>Cortar(&amp;X)</translation>
    </message>
    <message>
        <source>Ctrl+X</source>
        <translation>Ctrl+X</translation>
    </message>
    <message>
        <source>&amp;Paste</source>
        <translation>&amp;Pegar</translation>
    </message>
    <message>
        <source>Ctrl+V</source>
        <translation>Ctrl+V</translation>
    </message>
    <message>
        <source>Format Actions</source>
        <translation>Acciones de formato</translation>
    </message>
    <message>
        <source>&amp;Bold</source>
        <translation>&amp;Negrita</translation>
    </message>
    <message>
        <source>Ctrl+B</source>
        <translation>Ctrl+B</translation>
    </message>
    <message>
        <source>&amp;Italic</source>
        <translation>&amp;Cursiva</translation>
    </message>
    <message>
        <source>Ctrl+I</source>
        <translation>Ctrl+I</translation>
    </message>
    <message>
        <source>&amp;Underline</source>
        <translation>&amp;Subrayado</translation>
    </message>
    <message>
        <source>Ctrl+U</source>
        <translation>Ctrl+U</translation>
    </message>
    <message>
        <source>&amp;Left</source>
        <translation>&amp;Izquierda</translation>
    </message>
    <message>
        <source>Ctrl+L</source>
        <translation>Ctrl+L</translation>
    </message>
    <message>
        <source>C&amp;enter</source>
        <translation>C&amp;entrar</translation>
    </message>
    <message>
        <source>Ctrl+E</source>
        <translation>Ctrl+E</translation>
    </message>
    <message>
        <source>&amp;Right</source>
        <translation>&amp;Derecha</translation>
    </message>
    <message>
        <source>Ctrl+R</source>
        <translation>Ctrl+R</translation>
    </message>
    <message>
        <source>&amp;Justify</source>
        <translation>&amp;Justificar</translation>
    </message>
    <message>
        <source>Ctrl+J</source>
        <translation>Ctrl+J</translation>
    </message>
    <message>
        <source>&amp;Color...</source>
        <translation>&amp;Color...</translation>
    </message>
</context>
<context>
    <name>renameDialog</name>
    <message>
        <source>Rename...</source>
        <translation>Renombrar...</translation>
    </message>
</context>
<context>
    <name>setupDialog</name>
    <message>
        <source>Setup LMMS</source>
        <translation>Configuración de LMMS</translation>
    </message>
    <message>
        <source>General settings</source>
        <translation>Configuración General</translation>
    </message>
    <message>
        <source>BUFFER SIZE</source>
        <translation>Tamaño BUFFER</translation>
    </message>
    <message>
        <source>Reset to default-value</source>
        <translation>Restablecer los valores predeterminados</translation>
    </message>
    <message>
        <source>MISC</source>
        <translation>VARIOS</translation>
    </message>
    <message>
        <source>Enable tooltips</source>
        <translation>Activar ayuda contextual</translation>
    </message>
    <message>
        <source>Show restart warning after changing settings</source>
        <translation>Mostrar advertencia de reinicio para efectivizar los cambios de configuración</translation>
    </message>
    <message>
        <source>Display volume as dBV </source>
        <translation>Mostrar volumen como dBV</translation>
    </message>
    <message>
        <source>Compress project files per default</source>
        <translation>Comprimir archivos de proyecto por omisión</translation>
    </message>
    <message>
        <source>HQ-mode for output audio-device</source>
        <translation>modo HQ para salida de dispositivos de audio</translation>
    </message>
    <message>
        <source>LMMS working directory</source>
        <translation>Carpeta de trabajo LMMS</translation>
    </message>
    <message>
        <source>VST-plugin directory</source>
        <translation>Carpeta de Complementos VST</translation>
    </message>
    <message>
        <source>Artwork directory</source>
        <translation>Carpeta de Arte</translation>
    </message>
    <message>
        <source>FL Studio installation directory</source>
        <translation>Directorio de instalación de FL Studio</translation>
    </message>
    <message>
        <source>STK rawwave directory</source>
        <translation>Carpeta de STK rawwave</translation>
    </message>
    <message>
        <source>Performance settings</source>
        <translation>Configuración de Performance</translation>
    </message>
    <message>
        <source>UI effects vs. performance</source>
        <translation>Efectos vs. performance</translation>
    </message>
    <message>
        <source>Audio settings</source>
        <translation>Ajustes de Audio</translation>
    </message>
    <message>
        <source>AUDIO INTERFACE</source>
        <translation>INTERFAZ DE AUDIO</translation>
    </message>
    <message>
        <source>MIDI settings</source>
        <translation>Configuraciones MIDI</translation>
    </message>
    <message>
        <source>MIDI INTERFACE</source>
        <translation>INTERFAZ MIDI</translation>
    </message>
    <message>
        <source>OK</source>
        <translation>OK</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation>Cancelar</translation>
    </message>
    <message>
        <source>Restart LMMS</source>
        <translation>Reiniciar LMMS</translation>
    </message>
    <message>
        <source>Please note that most changes won&apos;t take effect until you restart LMMS!</source>
        <translation>Tenga en cuenta que la mayoría de los cambios no tendrán efecto hasta que reinicie LMMS!</translation>
    </message>
    <message>
        <source>Frames: %1
Latency: %2 ms</source>
        <translation>Cuadros: %1
Latencia: %2 ms</translation>
    </message>
    <message>
        <source>Here you can setup the internal buffer-size used by LMMS. Smaller values result in a lower latency but also may cause unusable sound or bad performance, especially on older computers or systems with a non-realtime kernel.</source>
        <translation>Acá podés configurar el tamaño del búfer interno utilizado por LMMS. Los valores más bajos resultan en una menor latencia, pero también pueden causar sonido inutilizable o mal desempeño, sobre todo en computadoras antiguas o en sistemas con un kernel no compatible con tiempo real.</translation>
    </message>
    <message>
        <source>Choose LMMS working directory</source>
        <translation>Elegir carpeta de trabajo de LMMS</translation>
    </message>
    <message>
        <source>Choose your VST-plugin directory</source>
        <translation>Elegir carpeta de Complementos VST</translation>
    </message>
    <message>
        <source>Choose artwork-theme directory</source>
        <translation>Elegí la carpeta de tema de arte</translation>
    </message>
    <message>
        <source>Choose FL Studio installation directory</source>
        <translation>Elegir directorio de instalación de FL Studio</translation>
    </message>
    <message>
        <source>Choose LADSPA plugin directory</source>
        <translation>Elegir carpeta de Complementos LADSPA</translation>
    </message>
    <message>
        <source>Choose STK rawwave directory</source>
        <translation>Elija carpeta rawwave STK</translation>
    </message>
    <message>
        <source>Here you can select your preferred audio-interface. Depending on the configuration of your system during compilation time you can choose between ALSA, JACK, OSS and more. Below you see a box which offers controls to setup the selected audio-interface.</source>
        <translation>Acá podés seleccionar tu interfaz de audio preferida. Dependiendo de la configuración de su sistema durante el tiempo de compilación se puede elegir entre ALSA, JACK, OSS y más. A continuación aparece un cuadro que ofrece controles para configurar la interfaz de audio seleccionada.</translation>
    </message>
    <message>
        <source>Here you can select your preferred MIDI-interface. Depending on the configuration of your system during compilation time you can choose between ALSA, OSS and more. Below you see a box which offers controls to setup the selected MIDI-interface.</source>
        <translation>Acá podés seleccionar tu interfaz MIDI preferida. Dependiendo de la configuración de tu sistema durante el tiempo de compilación se puede elegir entre ALSA, OSS y más. A continuación aparece un cuadro que ofrece controles para configurar la interfaz MIDI seleccionada.</translation>
    </message>
    <message>
        <source>Paths</source>
        <translation>Parches</translation>
    </message>
    <message>
        <source>LADSPA plugin paths</source>
        <translation>Ubicación de Complementos LADSPA</translation>
    </message>
    <message>
        <source>Default Soundfont File</source>
        <translation>Archivo SoundFont por omisión</translation>
    </message>
    <message>
        <source>Background artwork</source>
        <translation>Arte de fondo</translation>
    </message>
    <message>
        <source>Choose default SoundFont</source>
        <translation>Elegir SoundFont por defecto</translation>
    </message>
    <message>
        <source>Choose background artwork</source>
        <translation>Elegir arte de fondo</translation>
    </message>
    <message>
        <source>One instrument track window mode</source>
        <translation>Modo Ventana-pista-Un instrumento</translation>
    </message>
    <message>
        <source>Compact track buttons</source>
        <translation>Botones de pista compactos</translation>
    </message>
    <message>
        <source>Sync VST plugins to host playback</source>
        <translation>Plugins VST de sincronización para albergar la reproducción</translation>
    </message>
    <message>
        <source>Enable note labels in piano roll</source>
        <translation>Activar etiquetas de nota en PianoRoll</translation>
    </message>
    <message>
        <source>Enable waveform display by default</source>
        <translation>Activar mostrar forma de onda por omisión</translation>
    </message>
    <message>
        <source>Smooth scroll in Song Editor</source>
        <translation>Desplazamiento suave en Editor de Canciones</translation>
    </message>
    <message>
        <source>Enable auto save feature</source>
        <translation>Activar función Auto-Guardado</translation>
    </message>
    <message>
        <source>Show playback cursor in AudioFileProcessor</source>
        <translation>Mostrar cursor de reproducción en el instrumento &quot;AudioFileProcesor&quot;</translation>
    </message>
</context>
<context>
    <name>sf2Instrument</name>
    <message>
        <source>Bank</source>
        <translation>Banco</translation>
    </message>
    <message>
        <source>Patch</source>
        <translation>Parche</translation>
    </message>
    <message>
        <source>Gain</source>
        <translation>Ganancia</translation>
    </message>
    <message>
        <source>Reverb</source>
        <translation>Reverb</translation>
    </message>
    <message>
        <source>Reverb Roomsize</source>
        <translation>Reverb Tamaño de Habitación</translation>
    </message>
    <message>
        <source>Reverb Damping</source>
        <translation>Reverb Damping</translation>
    </message>
    <message>
        <source>Reverb Width</source>
        <translation>Reverb ancho</translation>
    </message>
    <message>
        <source>Reverb Level</source>
        <translation>Nivel de Reverb</translation>
    </message>
    <message>
        <source>Chorus</source>
        <translation>Coros</translation>
    </message>
    <message>
        <source>Chorus Lines</source>
        <translation>Líneas de Coros</translation>
    </message>
    <message>
        <source>Chorus Level</source>
        <translation>Nivel de Coros</translation>
    </message>
    <message>
        <source>Chorus Speed</source>
        <translation>Velocidad de Coros</translation>
    </message>
    <message>
        <source>Chorus Depth</source>
        <translation>Profundidad de Coros</translation>
    </message>
</context>
<context>
    <name>sf2InstrumentView</name>
    <message>
        <source>Open other SoundFont file</source>
        <translation>Abrir otro archivo SoundFont</translation>
    </message>
    <message>
        <source>Click here to open another SF2 file</source>
        <translation>Clic acá para abrir otro archivo SF2</translation>
    </message>
    <message>
        <source>Choose the patch</source>
        <translation>Elegir el parche</translation>
    </message>
    <message>
        <source>Gain</source>
        <translation>Ganancia</translation>
    </message>
    <message>
        <source>Apply reverb (if supported)</source>
        <translation>Aplicar Reverb (si es soportado)</translation>
    </message>
    <message>
        <source>This button enables the reverb effect. This is useful for cool effects, but only works on files that support it.</source>
        <translation>Este botón activa el efecto de Reverb. Esto es útil para efectos frescos, pero sólo funciona en archivos que lo soporten.</translation>
    </message>
    <message>
        <source>Reverb Roomsize:</source>
        <translation>Reverb Tamaño de Habitación:</translation>
    </message>
    <message>
        <source>Reverb Damping:</source>
        <translation>Reverb Damping:</translation>
    </message>
    <message>
        <source>Reverb Width:</source>
        <translation>Reverb ancho:</translation>
    </message>
    <message>
        <source>Reverb Level:</source>
        <translation>Nivel de Reverb:</translation>
    </message>
    <message>
        <source>Apply chorus (if supported)</source>
        <translation>Aplicar Coros (si es soportado)</translation>
    </message>
    <message>
        <source>This button enables the chorus effect. This is useful for cool echo effects, but only works on files that support it.</source>
        <translation>Este botón activa el efecto de coro. Esto es útil para efectos de eco fresco, pero sólo funciona en archivos que lo soporten.</translation>
    </message>
    <message>
        <source>Chorus Lines:</source>
        <translation>Líneas de Coros:</translation>
    </message>
    <message>
        <source>Chorus Level:</source>
        <translation>Nivel de Coros:</translation>
    </message>
    <message>
        <source>Chorus Speed:</source>
        <translation>Velocidad de Coros:</translation>
    </message>
    <message>
        <source>Chorus Depth:</source>
        <translation>Profundidad de Coros:</translation>
    </message>
    <message>
        <source>Open SoundFont file</source>
        <translation>Abrir archivo SoundFont</translation>
    </message>
    <message>
        <source>SoundFont2 Files (*.sf2)</source>
        <translation>Archivos SoundFont2 (*.sf2)</translation>
    </message>
</context>
<context>
    <name>sfxrInstrument</name>
    <message>
        <source>Wave Form</source>
        <translation>Forma de onda</translation>
    </message>
</context>
<context>
    <name>sidInstrument</name>
    <message>
        <source>Cutoff</source>
        <translation>Corte</translation>
    </message>
    <message>
        <source>Resonance</source>
        <translation>Resonancia</translation>
    </message>
    <message>
        <source>Filter type</source>
        <translation>Tipo de filtro</translation>
    </message>
    <message>
        <source>Voice 3 off</source>
        <translation>Voz 3 apagada</translation>
    </message>
    <message>
        <source>Volume</source>
        <translation>Volumen</translation>
    </message>
    <message>
        <source>Chip model</source>
        <translation>Modelo de Chip</translation>
    </message>
</context>
<context>
    <name>sidInstrumentView</name>
    <message>
        <source>Volume:</source>
        <translation>Volumen:</translation>
    </message>
    <message>
        <source>Resonance:</source>
        <translation>Resonancia:</translation>
    </message>
    <message>
        <source>Cutoff frequency:</source>
        <translation>Frecuencia de corte:</translation>
    </message>
    <message>
        <source>High-Pass filter </source>
        <translation>Filtro pasa-altos </translation>
    </message>
    <message>
        <source>Band-Pass filter </source>
        <translation>Filtro pasa-banda</translation>
    </message>
    <message>
        <source>Low-Pass filter </source>
        <translation>Filtro pasa-bajos </translation>
    </message>
    <message>
        <source>Voice3 Off </source>
        <translation>Voz 3 apagada</translation>
    </message>
    <message>
        <source>MOS6581 SID </source>
        <translation>MOS6581 SID</translation>
    </message>
    <message>
        <source>MOS8580 SID </source>
        <translation>MOS8580 SID</translation>
    </message>
    <message>
        <source>Attack:</source>
        <translation>Ataque:</translation>
    </message>
    <message>
        <source>Attack rate determines how rapidly the output of Voice %1 rises from zero to peak amplitude.</source>
        <translation>Tasa de ataque que determina la rapidez con que la salida de voz %1 se eleva de cero a pico de amplitud.</translation>
    </message>
    <message>
        <source>Decay:</source>
        <translation>Decay:</translation>
    </message>
    <message>
        <source>Decay rate determines how rapidly the output falls from the peak amplitude to the selected Sustain level.</source>
        <translation>La Tasa de Decaimiento determina la rapidez con que la salida cae de la amplitud máxima para el nivel de Sustain seleccionado.</translation>
    </message>
    <message>
        <source>Sustain:</source>
        <translation>Sustain:</translation>
    </message>
    <message>
        <source>Output of Voice %1 will remain at the selected Sustain amplitude as long as the note is held.</source>
        <translation>La Salida de Voz %1 se mantendrá en la amplitud de Sustain seleccionada mientras la nota se mantenga.</translation>
    </message>
    <message>
        <source>Release:</source>
        <translation>Release:</translation>
    </message>
    <message>
        <source>The output of of Voice %1 will fall from Sustain amplitude to zero amplitude at the selected Release rate.</source>
        <translation>La salida de la Voz %1 caerá desde la amplitud sustain a cero a la velocidad de lanzamiento seleccionada.</translation>
    </message>
    <message>
        <source>Pulse Width:</source>
        <translation>Ancho de pulso:</translation>
    </message>
    <message>
        <source>The Pulse Width resolution allows the width to be smoothly swept with no discernable stepping. The Pulse waveform on Oscillator %1 must be selected to have any audible effect.</source>
        <translation>La resolución de ancho de pulso permite barrer sin que se noten pasos intermedios. La forma de onda de pulso en el Oscilador %1 debe ser seleccionada para tener algún efecto audible.</translation>
    </message>
    <message>
        <source>Coarse:</source>
        <translation>Grueso:</translation>
    </message>
    <message>
        <source>The Coarse detuning allows to detune Voice %1 one octave up or down.</source>
        <translation>La desafinación gruesa permite desafinar Voz %1 una octava arriba o abajo.</translation>
    </message>
    <message>
        <source>Pulse Wave</source>
        <translation>Onda de pulso</translation>
    </message>
    <message>
        <source>Triangle Wave</source>
        <translation>onda-triangular</translation>
    </message>
    <message>
        <source>SawTooth</source>
        <translation>Diente de sierra</translation>
    </message>
    <message>
        <source>Noise</source>
        <translation>Ruido</translation>
    </message>
    <message>
        <source>Sync</source>
        <translation>Sincro</translation>
    </message>
    <message>
        <source>Sync synchronizes the fundamental frequency of Oscillator %1 with the fundamental frequency of Oscillator %2 producing &quot;Hard Sync&quot; effects.</source>
        <translation>/&quot;Sincro/&quot; sincroniza la frecuencia fundamental del oscilador %1 con la frecuencia fundamental de %2 la producción de efectos &quot;hard-sync&quot;.</translation>
    </message>
    <message>
        <source>Ring-Mod</source>
        <translation>Ring-Mod</translation>
    </message>
    <message>
        <source>Ring-mod replaces the Triangle Waveform output of Oscillator %1 with a &quot;Ring Modulated&quot; combination of Oscillators %1 and %2.</source>
        <translation>Ring-mod reemplaza la salida Triángulo de forma de onda del oscilador %1 con una combinación &quot;Anillo Modulada&quot; de osciladores %1 y %2.</translation>
    </message>
    <message>
        <source>Filtered</source>
        <translation>Filtrado</translation>
    </message>
    <message>
        <source>When Filtered is on, Voice %1 will be processed through the Filter. When Filtered is off, Voice %1 appears directly at the output, and the Filter has no effect on it.</source>
        <translation>Cuando se activa el Filtrado, La voz %1 será procesada a través del filtro. Cuando el Filtrado está desactivado, la voz %1 aparece directamente en la salida, sin efectos de filtro.</translation>
    </message>
    <message>
        <source>Test</source>
        <translation>Prueba</translation>
    </message>
    <message>
        <source>Test, when set, resets and locks Oscillator %1 at zero until Test is turned off.</source>
        <translation>Prueba:  al activar Prueba se reinicia el oscilador %1 en CERO hasta que Prueba sea desactivado.</translation>
    </message>
</context>
<context>
    <name>song</name>
    <message>
        <source>Tempo</source>
        <translation>Tiempo</translation>
    </message>
    <message>
        <source>Master volume</source>
        <translation>Volumen Maestro</translation>
    </message>
    <message>
        <source>Master pitch</source>
        <translation>Pitch maestro</translation>
    </message>
    <message>
        <source>Project saved</source>
        <translation>Proyecto guardado</translation>
    </message>
    <message>
        <source>The project %1 is now saved.</source>
        <translation>El proyecto %1 fue guardado.</translation>
    </message>
    <message>
        <source>Project NOT saved.</source>
        <translation>Proyecto NO guardado.</translation>
    </message>
    <message>
        <source>The project %1 was not saved!</source>
        <translation>El proyecto %1 NO fue guardado!</translation>
    </message>
    <message>
        <source>Import file</source>
        <translation>Importar archivo</translation>
    </message>
    <message>
        <source>untitled</source>
        <translation>Sin título</translation>
    </message>
    <message>
        <source>Select file for project-export...</source>
        <translation>Seleccione archivo para exportar proyecto...</translation>
    </message>
    <message>
        <source>Empty project</source>
        <translation>Proyecto vacío</translation>
    </message>
    <message>
        <source>This project is empty so exporting makes no sense. Please put some items into Song Editor first!</source>
        <translation>Este proyecto está vacio, por lo que exportar no tiene sentido. Deberías crear algo en el editor de Canción primero!</translation>
    </message>
    <message>
        <source>MIDI sequences</source>
        <translation>Secuencias MIDI</translation>
    </message>
    <message>
        <source>FL Studio projects</source>
        <translation>Proyectos FL Studio</translation>
    </message>
    <message>
        <source>All file types</source>
        <translation>Todos los tipos de archivo</translation>
    </message>
    <message>
        <source>Hydrogen projects</source>
        <translation>Proyectos Hydrogen</translation>
    </message>
    <message>
        <source>Select directory for writing exported tracks...</source>
        <translation>Seleccione carpeta para escribir pistas exportadas...</translation>
    </message>
</context>
<context>
    <name>SongEditor</name>
    <message>
        <source>Click here, if you want to stop playing of your song. The song-position-marker will be set to the start of your song.</source>
        <translation>Click aquí si usted desea detener la reproducción de su canción. El marcador de posición de la canción va a ser puesta al inicio de la canción.</translation>
    </message>
    <message>
        <source>Could not open file</source>
        <translation>No se puede abrir el archivo</translation>
    </message>
    <message>
        <source>Could not write file</source>
        <translation>No se puede escribir en el archivo</translation>
    </message>
    <message>
        <source>Song-Editor</source>
        <translation>Editor de canción</translation>
    </message>
    <message>
        <source>Click here, if you want to play your whole song. Playing will be started at the song-position-marker (green). You can also move it while playing.</source>
        <translation>Click aquí, si usted desea reproducir su canción completa. La reproducción se iniciara en el marcador de posición (verde). Usted puede también moverla mientras se reproduce.</translation>
    </message>
    <message>
        <source>Play song (Space)</source>
        <translation>Reproducir canción (Espaciador)</translation>
    </message>
    <message>
        <source>Stop song (Space)</source>
        <translation>Detener canción (Espaciador)</translation>
    </message>
    <message>
        <source>Add beat/bassline</source>
        <translation>Agregar beat/bassline</translation>
    </message>
    <message>
        <source>Add sample-track</source>
        <translation>Agregar pista de ejemplo</translation>
    </message>
    <message>
        <source>Add automation-track</source>
        <translation>Agregar pista de Automatización</translation>
    </message>
    <message>
        <source>Draw mode</source>
        <translation>Modo de Dibujo</translation>
    </message>
    <message>
        <source>Edit mode (select and move)</source>
        <translation>Modo Edición (seleccionar y mover)</translation>
    </message>
    <message>
        <source>Record samples from Audio-device</source>
        <translation>Grabar mustras desde un dispositivo de audio</translation>
    </message>
    <message>
        <source>Record samples from Audio-device while playing song or BB track</source>
        <translation>Grabar mustras desde un dispositivo de audio mientras se reproduce la canción</translation>
    </message>
    <message>
        <source>Could not open file %1. You probably have no permissions to read this file.
 Please make sure to have at least read permissions to the file and try again.</source>
        <translation>No se puede abrir el archivo %1 para lectura.
Por favor asegurate de que tenés permiso de escritura sobre el archivo y sobre la carpeta que contiene al archivo e ¡intentá nuevamente.</translation>
    </message>
    <message>
        <source>Error in file</source>
        <translation>Error en archivo</translation>
    </message>
    <message>
        <source>The file %1 seems to contain errors and therefore can&apos;t be loaded.</source>
        <translation>El archivo %1 parece contener errores y no puede ser cargado.</translation>
    </message>
    <message>
        <source>Tempo</source>
        <translation>Tiempo</translation>
    </message>
    <message>
        <source>TEMPO/BPM</source>
        <translation>TIEMPO/BPM</translation>
    </message>
    <message>
        <source>tempo of song</source>
        <translation>tiempo de canción</translation>
    </message>
    <message>
        <source>The tempo of a song is specified in beats per minute (BPM). If you want to change the tempo of your song, change this value. Every measure has four beats, so the tempo in BPM specifies, how many measures / 4 should be played within a minute (or how many measures should be played within four minutes).</source>
        <translation>La velocidad de una canción es especificada en Golpes por Minuto (beats per minute BPM). Si querés cambiar la velocidad de tu canción, cambiá este valor. Cada medida tiene cuatro golpes, entonces los BPM indican cuantas medidas / 4 deben ser tocadas en un minuto.</translation>
    </message>
    <message>
        <source>High quality mode</source>
        <translation>Modo Alta Calidad</translation>
    </message>
    <message>
        <source>Master volume</source>
        <translation>Volumen Maestro</translation>
    </message>
    <message>
        <source>master volume</source>
        <translation>Volumen Maestro</translation>
    </message>
    <message>
        <source>Master pitch</source>
        <translation>Pitch maestro</translation>
    </message>
    <message>
        <source>master pitch</source>
        <translation>Pitch maestro</translation>
    </message>
    <message>
        <source>Value: %1%</source>
        <translation>Valor: %1%</translation>
    </message>
    <message>
        <source>Value: %1 semitones</source>
        <translation>Valor: %1 semitonos</translation>
    </message>
    <message>
        <source>Could not open %1 for writing. You probably are not permitted to write to this file. Please make sure you have write-access to the file and try again.</source>
        <translation>No se puede abrir el archivo %1 para escritura.
Por favor asegurate de que tenés permiso de escritura sobre el archivo y sobre la carpeta que contiene al archivo e ¡intentá nuevamente.</translation>
    </message>
</context>
<context>
    <name>spectrumAnalyzerControlDialog</name>
    <message>
        <source>Linear spectrum</source>
        <translation>Espectro lineal</translation>
    </message>
    <message>
        <source>Linear Y axis</source>
        <translation>Lineal y axial</translation>
    </message>
</context>
<context>
    <name>spectrumAnalyzerControls</name>
    <message>
        <source>Linear spectrum</source>
        <translation>Espectro lineal</translation>
    </message>
    <message>
        <source>Linear Y-axis</source>
        <translation>Linear eje-Y</translation>
    </message>
    <message>
        <source>Channel mode</source>
        <translation>Modo de canal</translation>
    </message>
</context>
<context>
    <name>stereoEnhancerControlDialog</name>
    <message>
        <source>WIDE</source>
        <translation>AMPLIO</translation>
    </message>
    <message>
        <source>Width:</source>
        <translation>Ancho:</translation>
    </message>
</context>
<context>
    <name>stereoEnhancerControls</name>
    <message>
        <source>Width</source>
        <translation>Ancho</translation>
    </message>
</context>
<context>
    <name>stereoMatrixControlDialog</name>
    <message>
        <source>Left to Left Vol:</source>
        <translation>Izquiera a izquierda Vol:</translation>
    </message>
    <message>
        <source>Left to Right Vol:</source>
        <translation>Izquiera a derecha Vol:</translation>
    </message>
    <message>
        <source>Right to Left Vol:</source>
        <translation>Derecha a Izquiera Vol:</translation>
    </message>
    <message>
        <source>Right to Right Vol:</source>
        <translation>Derecha a derecha Vol:</translation>
    </message>
</context>
<context>
    <name>stereoMatrixControls</name>
    <message>
        <source>Left to Left</source>
        <translation>Izquiera a izquierda</translation>
    </message>
    <message>
        <source>Left to Right</source>
        <translation>Izquiera a derecha</translation>
    </message>
    <message>
        <source>Right to Left</source>
        <translation>Derecha a Izquiera</translation>
    </message>
    <message>
        <source>Right to Right</source>
        <translation>Derecha a derecha</translation>
    </message>
</context>
<context>
    <name>timeLine</name>
    <message>
        <source>Enable/disable auto-scrolling</source>
        <translation>Activar/desactivar función Auto-scroll</translation>
    </message>
    <message>
        <source>Enable/disable loop-points</source>
        <translation>Activar/desactivar puntos de bucle</translation>
    </message>
    <message>
        <source>After stopping go back to begin</source>
        <translation>Al parar, volver al inicio</translation>
    </message>
    <message>
        <source>After stopping go back to position at which playing was started</source>
        <translation>Al parar, volver a la posición donde se inició la reproducción</translation>
    </message>
    <message>
        <source>After stopping keep position</source>
        <translation>Al parar, mantener posición</translation>
    </message>
    <message>
        <source>Hint</source>
        <translation>TIP</translation>
    </message>
    <message>
        <source>Press &lt;Ctrl&gt; to disable magnetic loop points.</source>
        <translation>Presioná &lt;Ctrl&gt; para desactivar puntos de bucle magnéticos.</translation>
    </message>
    <message>
        <source>Hold &lt;Shift&gt; to move the begin loop point; Press &lt;Ctrl&gt; to disable magnetic loop points.</source>
        <translation>Mantené presionado &lt;Shift&gt; para mover el punto de inicio del bucle. Presioná &lt;Ctrl&gt; para desactivar puntos de bucle magnéticos.</translation>
    </message>
</context>
<context>
    <name>track</name>
    <message>
        <source>Muted</source>
        <translation>Silenciado</translation>
    </message>
    <message>
        <source>Solo</source>
        <translation>Solo</translation>
    </message>
</context>
<context>
    <name>trackContentObject</name>
    <message>
        <source>Muted</source>
        <translation>Silenciado</translation>
    </message>
</context>
<context>
    <name>trackContentObjectView</name>
    <message>
        <source>Current position</source>
        <translation>Posición actual</translation>
    </message>
    <message>
        <source>Hint</source>
        <translation>TIP</translation>
    </message>
    <message>
        <source>Press &lt;Ctrl&gt; and drag to make a copy.</source>
        <translation>Presione &lt;Ctrl&gt; y arrastre para hacer una copia.</translation>
    </message>
    <message>
        <source>Current length</source>
        <translation>Duración actual</translation>
    </message>
    <message>
        <source>Press &lt;Ctrl&gt; for free resizing.</source>
        <translation>Presione &lt;Ctrl&gt; para redimensionar libremente.</translation>
    </message>
    <message>
        <source>%1:%2 (%3:%4 to %5:%6)</source>
        <translation>%1:%2 (%3:%4 a %5:%6)</translation>
    </message>
    <message>
        <source>Delete (middle mousebutton)</source>
        <translation>Eliminar (botón medio del ratón)</translation>
    </message>
    <message>
        <source>Cut</source>
        <translation>Cortar</translation>
    </message>
    <message>
        <source>Copy</source>
        <translation>Copiar</translation>
    </message>
    <message>
        <source>Paste</source>
        <translation>Pegar</translation>
    </message>
    <message>
        <source>Mute/unmute (&lt;Ctrl&gt; + middle click)</source>
        <translation>Enmudecer/desenmudecer (&lt;Ctrl&gt;+botón medio)</translation>
    </message>
</context>
<context>
    <name>trackOperationsWidget</name>
    <message>
        <source>Press &lt;Ctrl&gt; while clicking on move-grip to begin a new drag&apos;n&apos;drop-action.</source>
        <translation>Presioná &lt;Ctrl&gt; mientras cliqueás en el movimiento de agarre para comenzar una nueva acción de arrastrar y soltar.</translation>
    </message>
    <message>
        <source>Actions for this track</source>
        <translation>Acciones para esta pista</translation>
    </message>
    <message>
        <source>Mute</source>
        <translation>Silenciar</translation>
    </message>
    <message>
        <source>Mute this track</source>
        <translation>Silenciar esta pista</translation>
    </message>
    <message>
        <source>Solo</source>
        <translation>Solo</translation>
    </message>
    <message>
        <source>Clone this track</source>
        <translation>Duplicar esta pista</translation>
    </message>
    <message>
        <source>Remove this track</source>
        <translation>Eliminar esta pista</translation>
    </message>
</context>
<context>
    <name>vestigeInstrument</name>
    <message>
        <source>Loading plugin</source>
        <translation>Cargando complemento</translation>
    </message>
    <message>
        <source>Please wait while loading VST-plugin...</source>
        <translation>Por favor espere, cargando complemento VST...</translation>
    </message>
    <message>
        <source>Failed loading VST-plugin</source>
        <translation>Error al cargar el complemento VST</translation>
    </message>
    <message>
        <source>The VST-plugin %1 could not be loaded for some reason.
If it runs with other VST-software under Linux, please contact an LMMS-developer!</source>
        <translation>El complemento VST %1 no pudo ser cargado por alguna razón. Si esto ocurre con otros complementos VST bajo gnu/linux, por favor contacte a un desarrollador de LMMS!</translation>
    </message>
</context>
<context>
    <name>vibed</name>
    <message>
        <source>String %1 volume</source>
        <translation>Volumen de cuerda %1</translation>
    </message>
    <message>
        <source>String %1 stiffness</source>
        <translation>Rigidez de cuerda %1</translation>
    </message>
    <message>
        <source>Pick %1 position</source>
        <translation>Escoja posición %1</translation>
    </message>
    <message>
        <source>Pickup %1 position</source>
        <translation>Posición de agarre %1</translation>
    </message>
    <message>
        <source>Pan %1</source>
        <translation>Pan %1</translation>
    </message>
    <message>
        <source>Detune %1</source>
        <translation>Desafinar %1</translation>
    </message>
    <message>
        <source>Fuzziness %1 </source>
        <translation>Tolerancia %1 </translation>
    </message>
    <message>
        <source>Length %1</source>
        <translation>Duración %1</translation>
    </message>
    <message>
        <source>Impulse %1</source>
        <translation>Impulso %1</translation>
    </message>
    <message>
        <source>Octave %1</source>
        <translation>Octava %1</translation>
    </message>
</context>
<context>
    <name>vibedView</name>
    <message>
        <source>Volume:</source>
        <translation>Volumen:</translation>
    </message>
    <message>
        <source>The &apos;V&apos; knob sets the volume of the selected string.</source>
        <translation>El control &quot;V&quot; ajusta el volumen para la cuerda seleccionada.</translation>
    </message>
    <message>
        <source>String stiffness:</source>
        <translation>Rigidez de cuerda:</translation>
    </message>
    <message>
        <source>The &apos;S&apos; knob sets the stiffness of the selected string.  The stiffness of the string affects how long the string will ring out.  The lower the setting, the longer the string will ring.</source>
        <translation>El mando de &apos;S&apos; establece la rigidez de la cuerda seleccionada. La rigidez de la cuerda afecta cuánto tiempo debe sonar. Cuanto menor sea el ajuste, más durará sonando la cuerda.</translation>
    </message>
    <message>
        <source>Pick position:</source>
        <translation>Posición de la selección:</translation>
    </message>
    <message>
        <source>The &apos;P&apos; knob sets the position where the selected string will be &apos;picked&apos;.  The lower the setting the closer the pick is to the bridge.</source>
        <translation>La perilla &apos;P&apos; establece la posición donde será tomada la cuerda. Cuanto menor sea el valor, más cerca la elección al puente.</translation>
    </message>
    <message>
        <source>Pickup position:</source>
        <translation>Posición de agarre:</translation>
    </message>
    <message>
        <source>The &apos;PU&apos; knob sets the position where the vibrations will be monitored for the selected string.  The lower the setting, the closer the pickup is to the bridge.</source>
        <translation>La perilla &apos;P&apos; establece la posición donde las vibraciones serán monitoreadas para la cuerda seleccionada. Cuanto menor el valor, más cercano el agarre al puente.</translation>
    </message>
    <message>
        <source>Pan:</source>
        <translation>Paneo:</translation>
    </message>
    <message>
        <source>The Pan knob determines the location of the selected string in the stereo field.</source>
        <translation>El mando Pan determina la ubicación de la cuerda seleccionada en el campo estéreo.</translation>
    </message>
    <message>
        <source>Detune:</source>
        <translation>Desafinar:</translation>
    </message>
    <message>
        <source>The Detune knob modifies the pitch of the selected string.  Settings less than zero will cause the string to sound flat.  Settings greater than zero will cause the string to sound sharp.</source>
        <translation>La perilla de desafinación modifica el tono de la cuerda seleccionada. Ajustes menores de cero hará que la cuerda suene plana. Ajustes mayores que cero harán que la cuerda suene aguda.</translation>
    </message>
    <message>
        <source>Fuzziness:</source>
        <translation>Tolerancia:</translation>
    </message>
    <message>
        <source>The Slap knob adds a bit of fuzz to the selected string which is most apparent during the attack, though it can also be used to make the string sound more &apos;metallic&apos;.</source>
        <translation>La perilla Slap añade efecto a la cuerda seleccionada, el cual es más evidente durante el ataque, aunque también puede ser utilizada para darle a la cuerda un sonido más &quot;metálico&quot;.</translation>
    </message>
    <message>
        <source>Length:</source>
        <translation>Duración:</translation>
    </message>
    <message>
        <source>The Length knob sets the length of the selected string.  Longer strings will both ring longer and sound brighter, however, they will also eat up more CPU cycles.</source>
        <translation>La perilla Longitud establece la longitud de la cuerda seleccionada. Cuerdas más largas sonarán más brillantes, sin embargo ello consume más ciclos de CPU.</translation>
    </message>
    <message>
        <source>Impulse or initial state</source>
        <translation>Impulso o estado inicial</translation>
    </message>
    <message>
        <source>The &apos;Imp&apos; selector determines whether the waveform in the graph is to be treated as an impulse imparted to the string by the pick or the initial state of the string.</source>
        <translation>El selector &apos;Imp&apos; determina si la forma de onda en el gráfico es para ser tratada como un impulso impartido a la cadena por la selección o el estado inicial de la cuerda.</translation>
    </message>
    <message>
        <source>Octave</source>
        <translation>Octava</translation>
    </message>
    <message>
        <source>The Octave selector is used to choose which harmonic of the note the string will ring at.  For example, &apos;-2&apos; means the string will ring two octaves below the fundamental, &apos;F&apos; means the string will ring at the fundamental, and &apos;6&apos; means the string will ring six octaves above the fundamental.</source>
        <translation>El selector de octava se utiliza para elegir el armónico de la nota en el que sonará la cuerda. Por ejemplo, &quot;-2&quot; significa la cuerda sonará dos octavas por debajo de la fundamental, &quot;F&quot; significa la cuerda sonará a la fundamental, y &apos;6&apos; significa la cuerda sonará seis octavas por encima de la fundamental.</translation>
    </message>
    <message>
        <source>Impulse Editor</source>
        <translation>Editor de Impulso</translation>
    </message>
    <message>
        <source>The waveform editor provides control over the initial state or impulse that is used to start the string vibrating.  The buttons to the right of the graph will initialize the waveform to the selected type.  The &apos;?&apos; button will load a waveform from a file--only the first 128 samples will be loaded.

The waveform can also be drawn in the graph.

The &apos;S&apos; button will smooth the waveform.

The &apos;N&apos; button will normalize the waveform.</source>
        <translation>El editor de forma de onda proporciona control sobre el estado inicial o de impulso que se utiliza para iniciar la cuerda vibrante. Los botones situados a la derecha del gráfico inicializarán la forma de onda del tipo seleccionado. Los botones &apos;?&apos; cargarán una forma de onda de un archivo - sólo se cargan las primeras 128 muestras.

La forma de onda también se puede dibujar en el gráfico.

El botón &apos;S&apos; suavizará la forma de onda.

El botón &apos;N&apos; normalizará la forma de onda.</translation>
    </message>
    <message>
        <source>Vibed models up to nine independently vibrating strings.  The &apos;String&apos; selector allows you to choose which string is being edited.  The &apos;Imp&apos; selector chooses whether the graph represents an impulse or the initial state of the string.  The &apos;Octave&apos; selector chooses which harmonic the string should vibrate at.

The graph allows you to control the initial state or impulse used to set the string in motion.

The &apos;V&apos; knob controls the volume.  The &apos;S&apos; knob controls the string&apos;s stiffness.  The &apos;P&apos; knob controls the pick position.  The &apos;PU&apos; knob controls the pickup position.

&apos;Pan&apos; and &apos;Detune&apos; hopefully don&apos;t need explanation.  The &apos;Slap&apos; knob adds a bit of fuzz to the sound of the string.

The &apos;Length&apos; knob controls the length of the string.

The LED in the lower right corner of the waveform editor determines whether the string is active in the current instrument.</source>
        <translation></translation>
    </message>
    <message>
        <source>Enable waveform</source>
        <translation>Activar forma de onda</translation>
    </message>
    <message>
        <source>Click here to enable/disable waveform.</source>
        <translation>Clic acá para activar/desactivar forma de onda.</translation>
    </message>
    <message>
        <source>String</source>
        <translation>Cuerda</translation>
    </message>
    <message>
        <source>The String selector is used to choose which string the controls are editing.  A Vibed instrument can contain up to nine independently vibrating strings.  The LED in the lower right corner of the waveform editor indicates whether the selected string is active.</source>
        <translation>El selector de cuerda se utiliza para elegir qué cuerda están editando los controles. Un instrumento vibed puede contener hasta nueve cuerdas que vibran de forma independiente. El LED en la esquina inferior derecha del editor de forma de onda indica si la cuerda seleccionada está activa.</translation>
    </message>
    <message>
        <source>Sine wave</source>
        <translation>onda-senoidal</translation>
    </message>
    <message>
        <source>Triangle wave</source>
        <translation>onda-triangular</translation>
    </message>
    <message>
        <source>Saw wave</source>
        <translation>onda-sierra</translation>
    </message>
    <message>
        <source>Square wave</source>
        <translation>onda-cuadrada</translation>
    </message>
    <message>
        <source>White noise wave</source>
        <translation>onda-ruido-blanco</translation>
    </message>
    <message>
        <source>User defined wave</source>
        <translation>onda-definida por el usuario</translation>
    </message>
    <message>
        <source>Smooth</source>
        <translation>Suave</translation>
    </message>
    <message>
        <source>Click here to smooth waveform.</source>
        <translation>Clic acá para suavizar forma de onda.</translation>
    </message>
    <message>
        <source>Normalize</source>
        <translation>Normalizar</translation>
    </message>
    <message>
        <source>Click here to normalize waveform.</source>
        <translation>Clic acá para normalizar forma de onda.</translation>
    </message>
    <message>
        <source>&amp;Help</source>
        <translation>&amp;Ayuda</translation>
    </message>
    <message>
        <source>Use a sine-wave for current oscillator.</source>
        <translation>Utilice una onda sinusoidal para el oscilador actual.</translation>
    </message>
    <message>
        <source>Use a triangle-wave for current oscillator.</source>
        <translation>Utilice una onda triangular para el oscilador actual.</translation>
    </message>
    <message>
        <source>Use a saw-wave for current oscillator.</source>
        <translation>Utilice una onda de sierra para el oscilador actual.</translation>
    </message>
    <message>
        <source>Use a square-wave for current oscillator.</source>
        <translation>Utilice una onda cuadradal para el oscilador actual.</translation>
    </message>
    <message>
        <source>Use white-noise for current oscillator.</source>
        <translation>Utilice ruido blanco para el oscilador actual.</translation>
    </message>
    <message>
        <source>Use a user-defined waveform for current oscillator.</source>
        <translation>Utilice una onda definida por el usuario para el oscilador actual.</translation>
    </message>
</context>
<context>
    <name>visualizationWidget</name>
    <message>
        <source>click to enable/disable visualization of master-output</source>
        <translation>click, para activar/desactivar visualización de la salida principal</translation>
    </message>
    <message>
        <source>Click to enable</source>
        <translatorcomment>Clic para activar</translatorcomment>
        <translation></translation>
    </message>
</context>
<context>
    <name>voiceObject</name>
    <message>
        <source>Voice %1 pulse width</source>
        <translation>Ancho de pulso Voz %1</translation>
    </message>
    <message>
        <source>Voice %1 attack</source>
        <translation>Ataque Voz %1</translation>
    </message>
    <message>
        <source>Voice %1 decay</source>
        <translation>Decay Voz %1</translation>
    </message>
    <message>
        <source>Voice %1 sustain</source>
        <translation>Sustento Voz %1</translation>
    </message>
    <message>
        <source>Voice %1 release</source>
        <translation>Liberación Voz %1</translation>
    </message>
    <message>
        <source>Voice %1 coarse detuning</source>
        <translation>Desafinación gruesa Voz %1</translation>
    </message>
    <message>
        <source>Voice %1 wave shape</source>
        <translation>Forma de onda Voz %1</translation>
    </message>
    <message>
        <source>Voice %1 sync</source>
        <translation>Sync Voz %1</translation>
    </message>
    <message>
        <source>Voice %1 ring modulate</source>
        <translation>Modulación Voz %1</translation>
    </message>
    <message>
        <source>Voice %1 filtered</source>
        <translation>Filtro Voz %1</translation>
    </message>
    <message>
        <source>Voice %1 test</source>
        <translation>Prueba Voz %1</translation>
    </message>
</context>
</TS>
