<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.0" language="ru_RU">
<context>
    <name>AboutDialog</name>
    <message>
        <source>About LMMS</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>LMMS (Linux MultiMedia Studio)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Version %1 (%2/%3, Qt %4, %5)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>About</source>
        <translation type="unfinished">О программе</translation>
    </message>
    <message>
        <source>LMMS - easy music production for everyone</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Authors</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Translation</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Current language not translated (or native English).

If you&apos;re interested in translating LMMS in another language or want to improve existing translations, you&apos;re welcome to help us! Simply contact the maintainer!</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>License</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Copyright (c) 2004-2014, LMMS developers</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;a href=&quot;http://lmms.sourceforge.net&quot;&gt;&lt;span style=&quot; text-decoration: underline; color:#0000ff;&quot;&gt;http://lmms.sourceforge.net&lt;/span&gt;&lt;/a&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>AudioAlsa::setupWidget</name>
    <message>
        <source>DEVICE</source>
        <translation type="unfinished">УСТРОЙСТВО</translation>
    </message>
    <message>
        <source>CHANNELS</source>
        <translation type="unfinished">ЧИСЛО КАНАЛОВ</translation>
    </message>
</context>
<context>
    <name>AudioFileProcessorView</name>
    <message>
        <source>Open other sample</source>
        <translation type="unfinished">Открыть другой образец</translation>
    </message>
    <message>
        <source>Click here, if you want to open another audio-file. A dialog will appear where you can select your file. Settings like looping-mode, start and end-points, amplify-value, and so on are not reset. So, it may not sound like the original sample.</source>
        <translation type="unfinished">Нажмите здесь, чтобы открыть другой звуковой файл. При этом появится стандартный диалог выбора файла. При этом сохраняются все настройки, такие как режим повтора, точки начала/конца, усиление.</translation>
    </message>
    <message>
        <source>Reverse sample</source>
        <translation type="unfinished">Перевернуть образец</translation>
    </message>
    <message>
        <source>If you enable this button, the whole sample is reversed. This is useful for cool effects, e.g. a reversed crash.</source>
        <translation type="unfinished">Перевернуть образец. Можно получать забавные эффекты - например, перевёрнутый взрыв.</translation>
    </message>
    <message>
        <source>Loop sample at start- and end-point</source>
        <translation type="unfinished">Постоянно воспроизводить участок образца между начальной и конечной точками</translation>
    </message>
    <message>
        <source>Here you can set, whether looping-mode is enabled. If enabled, AudioFileProcessor loops between start and end-points of a sample until the whole note is played. This is useful for things like string and choir samples.</source>
        <translation type="unfinished">Здесь вы можете указать, следует ли повторять звук при достижении конца файла. Если включено, то AudioFileProcessor будет повторять часть звука между начальной и конечной точками. Это может быть полезным для чего-нибудь вроде струнных или образцов аккордов.</translation>
    </message>
    <message>
        <source>Amplify:</source>
        <translation type="unfinished">Усиление:</translation>
    </message>
    <message>
        <source>With this knob you can set the amplify ratio. When you set a value of 100% your sample isn&apos;t changed. Otherwise it will be amplified up or down (your actual sample-file isn&apos;t touched!)</source>
        <translation type="unfinished">Эта ручка задаёт коэффициент усиления. При значении 100% исходный звук не меняется, если же нет ― он будет ослаблен или усилен. (Обратите внимание, что выбранный файл при этом останется нетронутым.)</translation>
    </message>
    <message>
        <source>Startpoint:</source>
        <translation type="unfinished">Начальная точка:</translation>
    </message>
    <message>
        <source>With this knob you can set the point where AudioFileProcessor should begin playing your sample. If you enable looping-mode, this is the point to which AudioFileProcessor returns if a note is longer than the sample between the start and end-points.</source>
        <translation type="unfinished">Эта ручка устанавливает точку, с которой будет начинаться воспроизведение образца. При включённом повторе модуль воспроизведения будет возвращаться к этой точке, если нота длительней части образца меж начальной и конечной точками.</translation>
    </message>
    <message>
        <source>Endpoint:</source>
        <translation type="unfinished">Конечная точка:</translation>
    </message>
    <message>
        <source>With this knob you can set the point where AudioFileProcessor should stop playing your sample. If you enable looping-mode, this is the point where AudioFileProcessor returns if a note is longer than the sample between the start and end-points.</source>
        <translation type="unfinished">Эта ручка устанавливает точку, где будет заканчиваться воспроизведение образца. При включённом повторе модуль воспроизведения по достижении этого места возвратится к начальной точке, если нота длительней части образца меж начальной и конечной точками.</translation>
    </message>
    <message>
        <source>Continue sample playback across notes</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Enabling this option makes the sample continue playing across different notes - if you change pitch, or the note length stops before the end of the sample, then the next note played will continue where it left off. To reset the playback to the start of the sample, insert a note at the bottom of the keyboard (&lt; 20 Hz)</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>AudioFileProcessorWaveView</name>
    <message>
        <source>Sample length:</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>AudioJack</name>
    <message>
        <source>JACK client restarted</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>LMMS was kicked by JACK for some reason. Therefore the JACK backend of LMMS has been restarted. You will have to make manual connections again.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>JACK server down</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>The JACK server seems to have been shutdown and starting a new instance failed. Therefore LMMS is unable to proceed. You should save your project and restart JACK and LMMS.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>AudioJack::setupWidget</name>
    <message>
        <source>CLIENT-NAME</source>
        <translation type="unfinished">ИМЯ КЛИЕНТА</translation>
    </message>
    <message>
        <source>CHANNELS</source>
        <translation type="unfinished">ЧИСЛО КАНАЛОВ</translation>
    </message>
</context>
<context>
    <name>AudioOss::setupWidget</name>
    <message>
        <source>DEVICE</source>
        <translation type="unfinished">УСТРОЙСТВО</translation>
    </message>
    <message>
        <source>CHANNELS</source>
        <translation type="unfinished">ЧИСЛО КАНАЛОВ</translation>
    </message>
</context>
<context>
    <name>AudioPortAudio::setupWidget</name>
    <message>
        <source>BACKEND</source>
        <translation type="unfinished">Драйвер</translation>
    </message>
    <message>
        <source>DEVICE</source>
        <translation type="unfinished">УСТРОЙСТВО</translation>
    </message>
</context>
<context>
    <name>AudioPulseAudio::setupWidget</name>
    <message>
        <source>DEVICE</source>
        <translation type="unfinished">УСТРОЙСТВО</translation>
    </message>
    <message>
        <source>CHANNELS</source>
        <translation type="unfinished">ЧИСЛО КАНАЛОВ</translation>
    </message>
</context>
<context>
    <name>AudioSdl::setupWidget</name>
    <message>
        <source>DEVICE</source>
        <translation type="unfinished">УСТРОЙСТВО</translation>
    </message>
</context>
<context>
    <name>AutomatableModel</name>
    <message>
        <source>&amp;Reset (%1%2)</source>
        <translation type="unfinished">&amp;Сбросить (%1%2)</translation>
    </message>
    <message>
        <source>&amp;Copy value (%1%2)</source>
        <translation type="unfinished">&amp;Копировать (%1%2)</translation>
    </message>
    <message>
        <source>&amp;Paste value (%1%2)</source>
        <translation type="unfinished">&amp;Вставить (%1%2)</translation>
    </message>
    <message>
        <source>Edit song-global automation</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Connected to %1</source>
        <translation type="unfinished">Подсоединено к %1</translation>
    </message>
    <message>
        <source>Connected to controller</source>
        <translation type="unfinished">подсоединено к контроллеру</translation>
    </message>
    <message>
        <source>Edit connection...</source>
        <translation type="unfinished">Настроить соединение...</translation>
    </message>
    <message>
        <source>Remove connection</source>
        <translation type="unfinished">Удалить соединение</translation>
    </message>
    <message>
        <source>Connect to controller...</source>
        <translation type="unfinished">Соединить с контроллером...</translation>
    </message>
    <message>
        <source>Remove song-global automation</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Remove all linked controls</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>AutomationEditor</name>
    <message>
        <source>Play/pause current pattern (Space)</source>
        <translation type="unfinished">Воспроизведение ткущего шаблона/пауза (Пробел)</translation>
    </message>
    <message>
        <source>Stop playing of current pattern (Space)</source>
        <translation type="unfinished">Остановить воспроизвдение шаблона (Пробел)</translation>
    </message>
    <message>
        <source>Click here if you want to play the current pattern. This is useful while editing it.  The pattern is automatically looped when the end is reached.</source>
        <translation type="unfinished">Нажмите здесь чтобы проиграть текущий шаблон. Это может пригодиться при его редактировании. Шаблон автоматически повторяется при достижении конца.</translation>
    </message>
    <message>
        <source>Click here if you want to stop playing of the current pattern.</source>
        <translation type="unfinished">Нажмите здесь, если вы хотите приостановить/продолжить воспроизведение текущего шаблона.</translation>
    </message>
    <message>
        <source>Draw mode (Shift+D)</source>
        <translation type="unfinished">Режим рисования (Shift+D)</translation>
    </message>
    <message>
        <source>Erase mode (Shift+E)</source>
        <translation type="unfinished">Режим стирания нот (Shift+E)</translation>
    </message>
    <message>
        <source>Click here and draw-mode will be activated. In this mode you can add and move single values.  This is the default mode which is used most of the time.  You can also press &apos;Shift+D&apos; on your keyboard to activate this mode.</source>
        <translation type="unfinished">При нажатии на эту кнопку активируется режим рисования нот, который является режимом по умолчанию и используется большую часть времени. В этом режиме вы можете добавлять/перемещать и изменять длительность одиночных нот.
Для включения этого режима можно использовать комбинацию клавиш Shift+D.</translation>
    </message>
    <message>
        <source>Click here and erase-mode will be activated. In this mode you can erase single values. You can also press &apos;Shift+E&apos; on your keyboard to activate this mode.</source>
        <translation type="unfinished">При нажатии на эту кнопку активируется режим стирания. В этом режиме вы можете стирать ноты по одной.
Для включения этого режима можно использовать комбинацию клавиш Shift+E.</translation>
    </message>
    <message>
        <source>Cut selected values (Ctrl+X)</source>
        <translation type="unfinished">Вырезать выбранные ноты (Ctrl+X)</translation>
    </message>
    <message>
        <source>Copy selected values (Ctrl+C)</source>
        <translation type="unfinished">Копировать выбранные ноты в буфер (Ctrl+C)</translation>
    </message>
    <message>
        <source>Paste values from clipboard (Ctrl+V)</source>
        <translation type="unfinished">Вставить значения из буфера (Ctrl+V)</translation>
    </message>
    <message>
        <source>Click here and selected values will be cut into the clipboard.  You can paste them anywhere in any pattern by clicking on the paste button.</source>
        <translation type="unfinished">При нажатии на эту кнопку выделеные ноты будут перемещены в буфер. Позже вы можете вставить их в любое место любого шаблона с помощью кнопки &quot;Вставить&quot;.</translation>
    </message>
    <message>
        <source>Click here and selected values will be copied into the clipboard.  You can paste them anywhere in any pattern by clicking on the paste button.</source>
        <translation type="unfinished">При нажатии на эту кнопку выделеные ноты будут скопированы в буфер. Позже вы можете вставить их в любое место любого шаблона с помощью кнопки &quot;Вставить&quot;.</translation>
    </message>
    <message>
        <source>Click here and the values from the clipboard will be pasted at the first visible measure.</source>
        <translation type="unfinished">При нажатии на эту кнопку ноты из буфера будут вставлены в превый видимый такт.</translation>
    </message>
    <message>
        <source>Automation Editor - no pattern</source>
        <translation type="unfinished">Редактор временных зависимостей — нет шаблона</translation>
    </message>
    <message>
        <source>Automation Editor - %1</source>
        <translation type="unfinished">Редактор временных зависимостей — %1</translation>
    </message>
    <message>
        <source>Please open an automation pattern with the context menu of a control!</source>
        <translation type="unfinished">Открывайте редатор временных зависимостей через контекстное меню регулятора!</translation>
    </message>
    <message>
        <source>Values copied</source>
        <translation type="unfinished">Значения скопированы</translation>
    </message>
    <message>
        <source>All selected values were copied to the clipboard.</source>
        <translation type="unfinished">Выбранные значения скопированы в буфер обмена.</translation>
    </message>
    <message>
        <source>Discrete progression</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Linear progression</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Cubic Hermite progression</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Tension: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Click here to choose discrete progressions for this automation pattern.  The value of the connected object will remain constant between control points and be set immediately to the new value when each control point is reached.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Click here to choose linear progressions for this automation pattern.  The value of the connected object will change at a steady rate over time between control points to reach the correct value at each control point without a sudden change.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Click here to choose cubic hermite progressions for this automation pattern.  The value of the connected object will change in a smooth curve and ease in to the peaks and valleys.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Tension value for spline</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>A higher tension value may make a smoother curve but overshoot some values.  A low tension value will cause the slope of the curve to level off at each control point.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>AutomationPattern</name>
    <message>
        <source>Drag a control while pressing &lt;Ctrl&gt;</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>AutomationPatternView</name>
    <message>
        <source>double-click to open this pattern in automation editor</source>
        <translation type="unfinished">Дважды щёлкните мышью чтобы настроить зависимость от времени для этого шаблона</translation>
    </message>
    <message>
        <source>Open in Automation editor</source>
        <translation type="unfinished">Настройка зависимости от времени</translation>
    </message>
    <message>
        <source>Clear</source>
        <translation type="unfinished">Очистить</translation>
    </message>
    <message>
        <source>Reset name</source>
        <translation type="unfinished">Сбросить название</translation>
    </message>
    <message>
        <source>Change name</source>
        <translation type="unfinished">Переименовать</translation>
    </message>
    <message>
        <source>%1 Connections</source>
        <translation type="unfinished">Соединения %1</translation>
    </message>
    <message>
        <source>Disconnect &quot;%1&quot;</source>
        <translation type="unfinished">Отсоединить «%1»</translation>
    </message>
</context>
<context>
    <name>AutomationTrack</name>
    <message>
        <source>Automation track</source>
        <translation type="unfinished">Дорожка автоматизации</translation>
    </message>
</context>
<context>
    <name>Controller</name>
    <message>
        <source>Controller %1</source>
        <translation type="unfinished">Контроллер %1</translation>
    </message>
</context>
<context>
    <name>ControllerConnectionDialog</name>
    <message>
        <source>Connection Settings</source>
        <translation type="unfinished">Параметры соединения</translation>
    </message>
    <message>
        <source>MIDI CONTROLLER</source>
        <translation type="unfinished">Контроллер MIDI</translation>
    </message>
    <message>
        <source>Input channel</source>
        <translation type="unfinished">Входной канал</translation>
    </message>
    <message>
        <source>CHANNEL</source>
        <translation type="unfinished">КАНАЛ</translation>
    </message>
    <message>
        <source>Input controller</source>
        <translation type="unfinished">Входной контроллер</translation>
    </message>
    <message>
        <source>CONTROLLER</source>
        <translation type="unfinished">КОНТРОЛЛЕР</translation>
    </message>
    <message>
        <source>Auto Detect</source>
        <translation type="unfinished">Автоопределение</translation>
    </message>
    <message>
        <source>MIDI-devices to receive MIDI-events from</source>
        <translation type="unfinished">Устройства MIDI, с которых нужно принимать события</translation>
    </message>
    <message>
        <source>USER CONTROLLER</source>
        <translation type="unfinished">ПОЛЬЗ. КОНТРОЛЛЕР</translation>
    </message>
    <message>
        <source>MAPPING FUNCTION</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>OK</source>
        <translation type="unfinished">УГУ</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation type="unfinished">Отменить</translation>
    </message>
    <message>
        <source>LMMS</source>
        <translation type="unfinished">LMMS</translation>
    </message>
    <message>
        <source>Cycle Detected.</source>
        <translation type="unfinished">Обнаружен цикл.</translation>
    </message>
</context>
<context>
    <name>ControllerRackView</name>
    <message>
        <source>Controller Rack</source>
        <translation type="unfinished">Контроллеры</translation>
    </message>
    <message>
        <source>Add</source>
        <translation type="unfinished">Добавить</translation>
    </message>
    <message>
        <source>Confirm Delete</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Confirm delete? There are existing connection(s) associted with this controller. There is no way to undo.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>ControllerView</name>
    <message>
        <source>Controls</source>
        <translation type="unfinished">Параметры</translation>
    </message>
    <message>
        <source>Controllers are able to automate the value of a knob, slider, and other controls.</source>
        <translation type="unfinished">Контроллеры служать для автоматизации изменений значения регуляторов, ползунков или других контроллеров.</translation>
    </message>
    <message>
        <source>Rename controller</source>
        <translation type="unfinished">Переименовать контроллер</translation>
    </message>
    <message>
        <source>Enter the new name for this controller</source>
        <translation type="unfinished">Введите новое название для контроллера</translation>
    </message>
    <message>
        <source>&amp;Remove this plugin</source>
        <translation type="unfinished">&amp;Убрать фильтр</translation>
    </message>
    <message>
        <source>&amp;Help</source>
        <translation type="unfinished">&amp;Справка</translation>
    </message>
</context>
<context>
    <name>Effect</name>
    <message>
        <source>Effect enabled</source>
        <translation type="unfinished">Эффект включён</translation>
    </message>
    <message>
        <source>Wet/Dry mix</source>
        <translation type="unfinished">Регулятор глубины фильтрации</translation>
    </message>
    <message>
        <source>Gate</source>
        <translation type="unfinished">Тишина</translation>
    </message>
    <message>
        <source>Decay</source>
        <translation type="unfinished">Длит. спада</translation>
    </message>
</context>
<context>
    <name>EffectChain</name>
    <message>
        <source>Effects enabled</source>
        <translation type="unfinished">Эффекты включёны</translation>
    </message>
</context>
<context>
    <name>EffectRackView</name>
    <message>
        <source>EFFECTS CHAIN</source>
        <translation type="unfinished">ЦЕПЬ ФИЛЬТРОВ</translation>
    </message>
    <message>
        <source>Add effect</source>
        <translation type="unfinished">Добавить фильтр</translation>
    </message>
</context>
<context>
    <name>EffectSelectDialog</name>
    <message>
        <source>Add effect</source>
        <translation type="unfinished">Добавить фильтр</translation>
    </message>
    <message>
        <source>Plugin description</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>EffectView</name>
    <message>
        <source>Toggles the effect on or off.</source>
        <translation type="unfinished">Включает/выключает фильтр.</translation>
    </message>
    <message>
        <source>On/Off</source>
        <translation type="unfinished">Вкл/Выкл</translation>
    </message>
    <message>
        <source>W/D</source>
        <translation type="unfinished">На фильтр</translation>
    </message>
    <message>
        <source>Wet Level:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>The Wet/Dry knob sets the ratio between the input signal and the effect signal that forms the output.</source>
        <translation type="unfinished">Регулятор глубины фильтрации определяет долю сигнала, изменяемую фильтром, т. е. которая часть выходного сигнала прошла через этот фильтр.</translation>
    </message>
    <message>
        <source>DECAY</source>
        <translation type="unfinished">СПАД</translation>
    </message>
    <message>
        <source>Time:</source>
        <translation type="unfinished">Задержка уничтожения фильтра:</translation>
    </message>
    <message>
        <source>The Decay knob controls how many buffers of silence must pass before the plugin stops processing.  Smaller values will reduce the CPU overhead but run the risk of clipping the tail on delay and reverb effects.</source>
        <translation type="unfinished">
Этот регулятор устанавливает время, которое будет действовать фильтр после последней ноты. Чем меньше значение, тем меньше требования к ЦП, однако это может вызвать обрезку звука при использовании эффектов задержки.</translation>
    </message>
    <message>
        <source>GATE</source>
        <translation type="unfinished">ЗАПОЛНЕНИЕ</translation>
    </message>
    <message>
        <source>Gate:</source>
        <translation type="unfinished">Уровень тишины:</translation>
    </message>
    <message>
        <source>The Gate knob controls the signal level that is considered to be &apos;silence&apos; while deciding when to stop processing signals.</source>
        <translation type="unfinished">Этот регулятор определяет наименьший уровень сигнала, который ещё будет посылаться фильтрам; всё, что ниже этого уровня, рассматриается как „тишина“ и не обрабатывается.</translation>
    </message>
    <message>
        <source>Controls</source>
        <translation type="unfinished">Параметры</translation>
    </message>
    <message>
        <source>Effect plugins function as a chained series of effects where the signal will be processed from top to bottom.

The On/Off switch allows you to bypass a given plugin at any point in time.

The Wet/Dry knob controls the balance between the input signal and the effected signal that is the resulting output from the effect.  The input for the stage is the output from the previous stage. So, the &apos;dry&apos; signal for effects lower in the chain contains all of the previous effects.

The Decay knob controls how long the signal will continue to be processed after the notes have been released.  The effect will stop processing signals when the volume has dropped below a given threshold for a given length of time.  This knob sets the &apos;given length of time&apos;.  Longer times will require more CPU, so this number should be set low for most effects.  It needs to be bumped up for effects that produce lengthy periods of silence, e.g. delays.

The Gate knob controls the &apos;given threshold&apos; for the effect&apos;s auto shutdown.  The clock for the &apos;given length of time&apos; will begin as soon as the processed signal level drops below the level specified with this knob.

The Controls button opens a dialog for editing the effect&apos;s parameters.

Right clicking will bring up a context menu where you can change the order in which the effects are processed or delete an effect altogether.</source>
        <translation type="unfinished">Сигнал проходит последовательно через все установленные фильтры (серху вниз).

Переключатель Вкл/Выкл позволяет в любой момент включать/выключать фильтр.

Регулятор глубины фильтрации определяет долю сигнала, изменяемую фильтром. Входной сигнал каждого фильтра является выходом предыдущего, так что доля нефильтрованного сигнала при прохождении по цепочке постоянно падает.

Регулятор длительности последействия фильтра определяет время, которое будет действовать фильтр после окончания последней ноты. Чем меньше значение, тем меньше требования к ЦП, однако это может вызвать обрезку звука при использовании эффектов задержки. Для большинства же эффектов его можно выбирать достаточно малым.

Регулятор уровня тишины служит для указания наименьшего уровня сигнала, который ещё будет посылаться фильтрам; всё, что ниже этого уровня, рассматриается как „тишина“ и не обрабатывается.

Кнопка „Параметры“ открывает окно, где вы можете передать параметры модулю.

Контекстное меню, вызываемое щелчком правой кнопкой мыши, позволяет менять порядок следования фильтров и удалять их.</translation>
    </message>
    <message>
        <source>Move &amp;up</source>
        <translation type="unfinished">Переместить &amp;выше</translation>
    </message>
    <message>
        <source>Move &amp;down</source>
        <translation type="unfinished">Переместить &amp;ниже</translation>
    </message>
    <message>
        <source>&amp;Remove this plugin</source>
        <translation type="unfinished">&amp;Убрать фильтр</translation>
    </message>
    <message>
        <source>&amp;Help</source>
        <translation type="unfinished">&amp;Справка</translation>
    </message>
</context>
<context>
    <name>EnvelopeAndLfoParameters</name>
    <message>
        <source>Predelay</source>
        <translation type="unfinished">Задержка</translation>
    </message>
    <message>
        <source>Attack</source>
        <translation type="unfinished">Длительность нарастания</translation>
    </message>
    <message>
        <source>Hold</source>
        <translation type="unfinished">Длительность гребня</translation>
    </message>
    <message>
        <source>Decay</source>
        <translation type="unfinished">Длит. спада</translation>
    </message>
    <message>
        <source>Sustain</source>
        <translation type="unfinished">Остаточный уровень</translation>
    </message>
    <message>
        <source>Release</source>
        <translation type="unfinished">Длительность исчезновения</translation>
    </message>
    <message>
        <source>Modulation</source>
        <translation type="unfinished">Модуляция</translation>
    </message>
    <message>
        <source>LFO Predelay</source>
        <translation type="unfinished">Задержка LFO</translation>
    </message>
    <message>
        <source>LFO Attack</source>
        <translation type="unfinished">Нарастание LFO</translation>
    </message>
    <message>
        <source>LFO speed</source>
        <translation type="unfinished">Скорость LFO</translation>
    </message>
    <message>
        <source>LFO Modulation</source>
        <translation type="unfinished">Модуляция LFO</translation>
    </message>
    <message>
        <source>LFO Wave Shape</source>
        <translation type="unfinished">Форма сигнала LFO</translation>
    </message>
    <message>
        <source>Freq x 100</source>
        <translation type="unfinished">ЧАСТ x 100</translation>
    </message>
    <message>
        <source>Modulate Env-Amount</source>
        <translation type="unfinished">Модулировать обложку</translation>
    </message>
</context>
<context>
    <name>EnvelopeAndLfoView</name>
    <message>
        <source>DEL</source>
        <translation type="unfinished">ЗДРЖ</translation>
    </message>
    <message>
        <source>Predelay:</source>
        <translation type="unfinished">Задержка:</translation>
    </message>
    <message>
        <source>Use this knob for setting predelay of the current envelope. The bigger this value the longer the time before start of actual envelope.</source>
        <translation type="unfinished">Этот регулятор определяет задержку перед воспроизведением звука.</translation>
    </message>
    <message>
        <source>ATT</source>
        <translation type="unfinished">НРСТ</translation>
    </message>
    <message>
        <source>Attack:</source>
        <translation type="unfinished">Длительность нарастания:</translation>
    </message>
    <message>
        <source>Use this knob for setting attack-time of the current envelope. The bigger this value the longer the envelope needs to increase to attack-level. Choose a small value for instruments like pianos and a big value for strings.</source>
        <translation type="unfinished">Этот регулятор устанавливает время возрастания для текущей обложки. Чем больше значение, тем дольше характеристика (н-р, громкость) возрастает до максимума. Для инструменов вроде пианино характерны малые времена нарастания, а для струнных - большие.</translation>
    </message>
    <message>
        <source>HOLD</source>
        <translation type="unfinished">HOLD</translation>
    </message>
    <message>
        <source>Hold:</source>
        <translation type="unfinished">Длительность гребня:</translation>
    </message>
    <message>
        <source>Use this knob for setting hold-time of the current envelope. The bigger this value the longer the envelope holds attack-level before it begins to decrease to sustain-level.</source>
        <translation type="unfinished">Эта ручка устанавливает длительность гребня волны текущей обложки. Чем больше значение, тем дольше обложка удерживает наибольший уровень.</translation>
    </message>
    <message>
        <source>DEC</source>
        <translation type="unfinished">СПАД</translation>
    </message>
    <message>
        <source>Decay:</source>
        <translation type="unfinished">Длительность спада:</translation>
    </message>
    <message>
        <source>Use this knob for setting decay-time of the current envelope. The bigger this value the longer the envelope needs to decrease from attack-level to sustain-level. Choose a small value for instruments like pianos.</source>
        <translation type="unfinished">Этот регулятор устанавливает время спада для текущей обложки. Чем больше значение, тем дольше уровень уменьшается от максимального  до остаточного. Для инструментов вроде пианино следует выбирать небольшие значения.</translation>
    </message>
    <message>
        <source>SUST</source>
        <translation type="unfinished">ОСТ</translation>
    </message>
    <message>
        <source>Sustain:</source>
        <translation type="unfinished">Остаточный уровень:</translation>
    </message>
    <message>
        <source>Use this knob for setting sustain-level of the current envelope. The bigger this value the higher the level on which the envelope stays before going down to zero.</source>
        <translation type="unfinished">Этот регулятор устанавливает остаточный уровень. Эта величина показывает, с какого уровня соответствующая характеристика снижается до нуля.</translation>
    </message>
    <message>
        <source>REL</source>
        <translation type="unfinished">ИСЧ</translation>
    </message>
    <message>
        <source>Release:</source>
        <translation type="unfinished">Длительность исчезновения:</translation>
    </message>
    <message>
        <source>Use this knob for setting release-time of the current envelope. The bigger this value the longer the envelope needs to decrease from sustain-level to zero. Choose a big value for soft instruments like strings.</source>
        <translation type="unfinished">Этот регулятор устанавливает время исчезновения для текущей обложки. Чем больше значение, тем дольше характеристика (н-р, громкость) уменьшается от остаточного уровня до нуля. Для струнных инструментов следует выбирать большие значения.</translation>
    </message>
    <message>
        <source>AMT</source>
        <translation type="unfinished">МОД</translation>
    </message>
    <message>
        <source>Modulation amount:</source>
        <translation type="unfinished">Глубина модуляции:</translation>
    </message>
    <message>
        <source>Use this knob for setting modulation amount of the current envelope. The bigger this value the more the according size (e.g. volume or cutoff-frequency) will be influenced by this envelope.</source>
        <translation type="unfinished">Этот регулятор устанавливает глубину модуляции для текущей обложки. Чем больше значение, тем в большей степени выбранная характеристика (н-р, громкость или частота среза) будет зависеть от обложки.</translation>
    </message>
    <message>
        <source>LFO predelay:</source>
        <translation type="unfinished">Пред. задержка LFO:</translation>
    </message>
    <message>
        <source>Use this knob for setting predelay-time of the current LFO. The bigger this value the the time until the LFO starts to oscillate.</source>
        <translation type="unfinished">Этот регулятор определяет задержку перед запуском LFO (LFO=низкочастотный генератор).</translation>
    </message>
    <message>
        <source>LFO- attack:</source>
        <translation type="unfinished">Нарастание LFO:</translation>
    </message>
    <message>
        <source>Use this knob for setting attack-time of the current LFO. The bigger this value the longer the LFO needs to increase its amplitude to maximum.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>SPD</source>
        <translation type="unfinished">СКОР</translation>
    </message>
    <message>
        <source>LFO speed:</source>
        <translation type="unfinished">Скорость LFO:</translation>
    </message>
    <message>
        <source>Use this knob for setting speed of the current LFO. The bigger this value the faster the LFO oscillates and the faster will be your effect.</source>
        <translation type="unfinished">Этот регулятор устанавлявает скорость текущего LFO.</translation>
    </message>
    <message>
        <source>Use this knob for setting modulation amount of the current LFO. The bigger this value the more the selected size (e.g. volume or cutoff-frequency) will be influenced by this LFO.</source>
        <translation type="unfinished">Этот регулятор устанавливает глубину модуляции для текущего LFO. Чем больше значение, тем в большей степени выбранная характеристика (н-р, громкость или частота среза) будет зависеть от LFO.</translation>
    </message>
    <message>
        <source>Click here for a sine-wave.</source>
        <translation type="unfinished">Генерировать гармонический (синусоидальный) сигнал.</translation>
    </message>
    <message>
        <source>Click here for a triangle-wave.</source>
        <translation type="unfinished">Сгенерировать треугольный сигнал.</translation>
    </message>
    <message>
        <source>Click here for a saw-wave for current.</source>
        <translation type="unfinished">Генерировать пилообразный сигнал.</translation>
    </message>
    <message>
        <source>Click here for a square-wave.</source>
        <translation type="unfinished">Сгенерировать меандр.</translation>
    </message>
    <message>
        <source>Click here for a user-defined wave. Afterwards, drag an according sample-file onto the LFO graph.</source>
        <translation type="unfinished">Задать форму сигнала из файла.</translation>
    </message>
    <message>
        <source>FREQ x 100</source>
        <translation type="unfinished">ЧАСТОТА x 100</translation>
    </message>
    <message>
        <source>Click here if the frequency of this LFO should be multiplied by 100.</source>
        <translation type="unfinished">Нажмите, чтобы увеличить частоту этого LFO в сто раз.</translation>
    </message>
    <message>
        <source>multiply LFO-frequency by 100</source>
        <translation type="unfinished">Увеличить частоту LFO в 100 раз</translation>
    </message>
    <message>
        <source>MODULATE ENV-AMOUNT</source>
        <translation type="unfinished">МОДУЛИРОВАТЬ ОБЛОЖКУ</translation>
    </message>
    <message>
        <source>Click here to make the envelope-amount controlled by this LFO.</source>
        <translation type="unfinished">Нажмите сюда, чтобы глубина модуляции обложки задавалась этим LFO.</translation>
    </message>
    <message>
        <source>control envelope-amount by this LFO</source>
        <translation type="unfinished">Разрешить этому LFO задавать глубину модуляции обложки</translation>
    </message>
    <message>
        <source>ms/LFO:</source>
        <translation type="unfinished">Период:</translation>
    </message>
    <message>
        <source>Hint</source>
        <translation type="unfinished">Подсказка</translation>
    </message>
    <message>
        <source>Drag a sample from somewhere and drop it in this window.</source>
        <translation type="unfinished">Перетащите в это окно какой-нибудь образец.</translation>
    </message>
</context>
<context>
    <name>ExportProjectDialog</name>
    <message>
        <source>Export project</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Output</source>
        <translation type="unfinished">Выход</translation>
    </message>
    <message>
        <source>File format:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Samplerate:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>44100 Hz</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>48000 Hz</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>88200 Hz</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>96000 Hz</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>192000 Hz</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Bitrate:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>64 KBit/s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>128 KBit/s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>160 KBit/s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>192 KBit/s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>256 KBit/s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>320 KBit/s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Depth:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>16 Bit Integer</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>32 Bit Float</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Please note that not all of the parameters above apply for all file formats.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Quality settings</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Interpolation:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Zero Order Hold</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Sinc Fastest</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Sinc Medium (recommended)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Sinc Best (very slow!)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Oversampling (use with care!):</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>1x (None)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>2x</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>4x</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>8x</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Sample-exact controllers</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Alias-free oscillators</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Start</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation type="unfinished">Отменить</translation>
    </message>
    <message>
        <source>Export as loop (remove end silence)</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>FxMixer</name>
    <message>
        <source>Master</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>FX %1</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>FxMixerView</name>
    <message>
        <source>Rename FX channel</source>
        <translation type="unfinished">Переименовать канал FX</translation>
    </message>
    <message>
        <source>Enter the new name for this FX channel</source>
        <translation type="unfinished">Введите новое название для канала</translation>
    </message>
    <message>
        <source>FX-Mixer</source>
        <translation type="unfinished">Микшер FX</translation>
    </message>
    <message>
        <source>FX Fader %1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Mute</source>
        <translation type="unfinished">Заглушить</translation>
    </message>
    <message>
        <source>Mute this FX channel</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>InstrumentFunctionArpeggio</name>
    <message>
        <source>Arpeggio</source>
        <translation type="unfinished">Арпеджио</translation>
    </message>
    <message>
        <source>Arpeggio type</source>
        <translation type="unfinished">Тип арпеджио</translation>
    </message>
    <message>
        <source>Arpeggio range</source>
        <translation type="unfinished">Частотный диапазон арпеджио</translation>
    </message>
    <message>
        <source>Arpeggio time</source>
        <translation type="unfinished">Период арпеджио</translation>
    </message>
    <message>
        <source>Arpeggio gate</source>
        <translation type="unfinished">Заполнение арпеджио</translation>
    </message>
    <message>
        <source>Arpeggio direction</source>
        <translation type="unfinished">Направление арпеджио</translation>
    </message>
    <message>
        <source>Arpeggio mode</source>
        <translation type="unfinished">Режим арпеджио</translation>
    </message>
    <message>
        <source>Up</source>
        <translation type="unfinished">Вверх</translation>
    </message>
    <message>
        <source>Down</source>
        <translation type="unfinished">Вниз</translation>
    </message>
    <message>
        <source>Up and down</source>
        <translation type="unfinished">Вверх и вниз</translation>
    </message>
    <message>
        <source>Random</source>
        <translation type="unfinished">Случайно</translation>
    </message>
    <message>
        <source>Free</source>
        <translation type="unfinished">Свободно</translation>
    </message>
    <message>
        <source>Sort</source>
        <translation type="unfinished">Упорядочить</translation>
    </message>
    <message>
        <source>Sync</source>
        <translation type="unfinished">Синхронизировать</translation>
    </message>
</context>
<context>
    <name>InstrumentFunctionArpeggioView</name>
    <message>
        <source>ARPEGGIO</source>
        <translation type="unfinished">АРПЕДЖИО</translation>
    </message>
    <message>
        <source>An arpeggio is a method playing (especially plucked) instruments, which makes the music much livelier. The strings of such instruments (e.g. harps) are plucked like chords. The only difference is that this is done in a sequential order, so the notes are not played at the same time. Typical arpeggios are major or minor triads, but there are a lot of other possible chords, you can select.</source>
        <translation type="unfinished">Арпджио — разновидность исполнения аккордов на фортепиано и струнных инструментах, как на арфе, когда звуки аккорда следуют один за другим.</translation>
    </message>
    <message>
        <source>RANGE</source>
        <translation type="unfinished">ДИАПАЗОН</translation>
    </message>
    <message>
        <source>Arpeggio range:</source>
        <translation type="unfinished">Диапазон арпеджио:</translation>
    </message>
    <message>
        <source>octave(s)</source>
        <translation type="unfinished">Октав[а/ы]</translation>
    </message>
    <message>
        <source>Use this knob for setting the arpeggio range in octaves. The selected arpeggio will be played within specified number of octaves.</source>
        <translation type="unfinished">Используйте ручку, чтобы установить диапазон арпеджио (в октавах). Выбранный тип арпеджио будет охватывать указанное количество октав.</translation>
    </message>
    <message>
        <source>TIME</source>
        <translation type="unfinished">ВРЕМЯ</translation>
    </message>
    <message>
        <source>Arpeggio time:</source>
        <translation type="unfinished">Период арпеджио:</translation>
    </message>
    <message>
        <source>ms</source>
        <translation type="unfinished">мс</translation>
    </message>
    <message>
        <source>Use this knob for setting the arpeggio time in milliseconds. The arpeggio time specifies how long each arpeggio-tone should be played.</source>
        <translation type="unfinished">Регулировка периода арпеджио - время (в миллисекундах), которое должнен звучать каждый тон арпеджио.</translation>
    </message>
    <message>
        <source>GATE</source>
        <translation type="unfinished">ЗАПОЛНЕНИЕ</translation>
    </message>
    <message>
        <source>Arpeggio gate:</source>
        <translation type="unfinished">Коэффициент заполнения:</translation>
    </message>
    <message>
        <source>%</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Use this knob for setting the arpeggio gate. The arpeggio gate specifies the percent of a whole arpeggio-tone that should be played. With this you can make cool staccato arpeggios.</source>
        <translation type="unfinished">Регулировка заполнения для арпеджио. Коэффициент заполнения показывает, какая часть каждого тона будет проиграна. Простой способ создавать стаккато-орпеджио.</translation>
    </message>
    <message>
        <source>Chord:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Direction:</source>
        <translation type="unfinished">Направление:</translation>
    </message>
    <message>
        <source>Mode:</source>
        <translation type="unfinished">Режим:</translation>
    </message>
</context>
<context>
    <name>InstrumentFunctionNoteStacking</name>
    <message>
        <source>octave</source>
        <translation type="unfinished">Октава</translation>
    </message>
    <message>
        <source>Major</source>
        <translation type="unfinished">Мажорный</translation>
    </message>
    <message>
        <source>Majb5</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>minor</source>
        <translation type="unfinished">Минорный</translation>
    </message>
    <message>
        <source>minb5</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>sus2</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>sus4</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>aug</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>augsus4</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>tri</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>6</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>6sus4</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>6add9</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>m6</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>m6add9</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>7</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>7sus4</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>7#5</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>7b5</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>7#9</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>7b9</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>7#5#9</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>7#5b9</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>7b5b9</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>7add11</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>7add13</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>7#11</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Maj7</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Maj7b5</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Maj7#5</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Maj7#11</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Maj7add13</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>m7</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>m7b5</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>m7b9</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>m7add11</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>m7add13</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>m-Maj7</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>m-Maj7add11</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>m-Maj7add13</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>9</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>9sus4</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>add9</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>9#5</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>9b5</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>9#11</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>9b13</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Maj9</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Maj9sus4</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Maj9#5</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Maj9#11</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>m9</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>madd9</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>m9b5</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>m9-Maj7</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>11</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>11b9</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Maj11</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>m11</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>m-Maj11</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>13</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>13#9</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>13b9</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>13b5b9</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Maj13</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>m13</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>m-Maj13</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Harmonic minor</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Melodic minor</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Whole tone</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Diminished</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Major pentatonic</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Minor pentatonic</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Jap in sen</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Major bebop</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Dominant bebop</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Blues</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Arabic</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Enigmatic</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Neopolitan</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Neopolitan minor</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Hungarian minor</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Dorian</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Phrygolydian</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Lydian</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Mixolydian</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Aeolian</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Locrian</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Chords</source>
        <translation type="unfinished">Аккорд</translation>
    </message>
    <message>
        <source>Chord type</source>
        <translation type="unfinished">Тип аккорда</translation>
    </message>
    <message>
        <source>Chord range</source>
        <translation type="unfinished">Частотный диапазон аккорда</translation>
    </message>
    <message>
        <source>Minor</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>InstrumentFunctionNoteStackingView</name>
    <message>
        <source>RANGE</source>
        <translation type="unfinished">ДИАПАЗОН</translation>
    </message>
    <message>
        <source>Chord range:</source>
        <translation type="unfinished">Диапазон аккорда:</translation>
    </message>
    <message>
        <source>octave(s)</source>
        <translation type="unfinished">Октав[а/ы]</translation>
    </message>
    <message>
        <source>Use this knob for setting the chord range in octaves. The selected chord will be played within specified number of octaves.</source>
        <translation type="unfinished">Этот регулятор изменяет частотный диапазон акорда: аккорд будет содержать указанное число октав.</translation>
    </message>
    <message>
        <source>STACKING</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Chord:</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>InstrumentMidiIOView</name>
    <message>
        <source>ENABLE MIDI INPUT</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>CHANNEL</source>
        <translation type="unfinished">КАНАЛ</translation>
    </message>
    <message>
        <source>VELOCITY</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>ENABLE MIDI OUTPUT</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>PROGRAM</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>MIDI devices to receive MIDI events from</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>MIDI devices to send MIDI events to</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>NOTE</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>InstrumentSoundShaping</name>
    <message>
        <source>VOLUME</source>
        <translation type="unfinished">ГРОМКОСТЬ</translation>
    </message>
    <message>
        <source>Volume</source>
        <translation type="unfinished">Громкость</translation>
    </message>
    <message>
        <source>CUTOFF</source>
        <translation type="unfinished">СРЕЗ</translation>
    </message>
    <message>
        <source>Cutoff frequency</source>
        <translation type="unfinished">Частота среза</translation>
    </message>
    <message>
        <source>RESO</source>
        <translation type="unfinished">УСИЛ</translation>
    </message>
    <message>
        <source>Resonance</source>
        <translation type="unfinished">Усиление</translation>
    </message>
    <message>
        <source>Envelopes/LFOs</source>
        <translation type="unfinished">Обложки/LFO</translation>
    </message>
    <message>
        <source>Filter type</source>
        <translation type="unfinished">Тип фильтра</translation>
    </message>
    <message>
        <source>Q/Resonance</source>
        <translation type="unfinished">Усиление</translation>
    </message>
    <message>
        <source>LowPass</source>
        <translation type="unfinished">ФНЧ</translation>
    </message>
    <message>
        <source>HiPass</source>
        <translation type="unfinished">ФВЧ</translation>
    </message>
    <message>
        <source>BandPass csg</source>
        <translation type="unfinished">ППФ csg</translation>
    </message>
    <message>
        <source>BandPass czpg</source>
        <translation type="unfinished">ППФ czpg</translation>
    </message>
    <message>
        <source>Notch</source>
        <translation type="unfinished">Полосно-заграждающий</translation>
    </message>
    <message>
        <source>Allpass</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Moog</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>2x LowPass</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>RC LowPass 12dB</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>RC BandPass 12dB</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>RC HighPass 12dB</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>RC LowPass 24dB</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>RC BandPass 24dB</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>RC HighPass 24dB</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Vocal Formant Filter</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>InstrumentSoundShapingView</name>
    <message>
        <source>TARGET</source>
        <translation type="unfinished">ХАРАКТЕРИСТИКА</translation>
    </message>
    <message>
        <source>These tabs contain envelopes. They&apos;re very important for modifying a sound, in that they are almost always necessary for substractive synthesis. For example if you have a volume envelope, you can set when the sound should have a specific volume. If you want to create some soft strings then your sound has to fade in and out very softly. This can be done by setting large attack and release times. It&apos;s the same for other envelope targets like panning, cutoff frequency for the used filter and so on. Just monkey around with it! You can really make cool sounds out of a saw-wave with just some envelopes...!</source>
        <translation type="unfinished">Эта вкладка позволяет вам настроить обложки. Они очень важны для настройки звучания.
Например, с помощью обложки громкости вы можете задать зависимость громкости звучания от времени. Если вам понадобится эмулировать мягкие струнные, просто задайте большие времена нарастания и исчезновения звука. С помощью обложек и низкочастотного генератора (LFO) вы в несколько щелчков мыши сможете создать просто невероятные звуки!</translation>
    </message>
    <message>
        <source>FILTER</source>
        <translation type="unfinished">ФИЛЬТР</translation>
    </message>
    <message>
        <source>Here you can select the built-in filter you want to use for this instrument-track. Filters are very important for changing the characteristics of a sound.</source>
        <translation type="unfinished">Здесь вы можете выбрать фильтр для этого инструмента. Фильтры могут довольно сильно менять звучание.</translation>
    </message>
    <message>
        <source>Hz</source>
        <translation type="unfinished">Гц</translation>
    </message>
    <message>
        <source>Use this knob for setting the cutoff frequency for the selected filter. The cutoff frequency specifies the frequency for cutting the signal by a filter. For example a lowpass-filter cuts all frequencies above the cutoff frequency. A highpass-filter cuts all frequencies below cutoff frequency, and so on...</source>
        <translation type="unfinished">Этот регулятор устанавливает частоту среза для выбранного фильтра. К примеру, ФНЧ будет срезать сигнал на частотах выше частоты среза, полосно-пропускающий фильтр будет хорошо пропускать сигнал только на заданной частоте и так далее...</translation>
    </message>
    <message>
        <source>RESO</source>
        <translation type="unfinished">УСИЛ</translation>
    </message>
    <message>
        <source>Resonance:</source>
        <translation type="unfinished">Усиление:</translation>
    </message>
    <message>
        <source>Use this knob for setting Q/Resonance for the selected filter. Q/Resonance tells the filter how much it should amplify frequencies near Cutoff-frequency.</source>
        <translation type="unfinished">Эта  ручка задаёт усиление сигнала на частота среза (резонансной частоте).</translation>
    </message>
    <message>
        <source>FREQ</source>
        <translation type="unfinished">ЧАСТ</translation>
    </message>
    <message>
        <source>cutoff frequency:</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>InstrumentTrack</name>
    <message>
        <source>unnamed_track</source>
        <translation type="unfinished">безымянный канал</translation>
    </message>
    <message>
        <source>Volume</source>
        <translation type="unfinished">Громкость</translation>
    </message>
    <message>
        <source>Panning</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Pitch</source>
        <translation type="unfinished">Сдвиг высоты</translation>
    </message>
    <message>
        <source>FX channel</source>
        <translation type="unfinished">Канал FX</translation>
    </message>
    <message>
        <source>Default preset</source>
        <translation type="unfinished">Не названо</translation>
    </message>
    <message>
        <source>With this knob you can set the volume of the opened channel.</source>
        <translation type="unfinished">Регулировка громкости текущего канала.</translation>
    </message>
    <message>
        <source>Base note</source>
        <translation type="unfinished">Опорная нота</translation>
    </message>
    <message>
        <source>Pitch range</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>InstrumentTrackView</name>
    <message>
        <source>Volume</source>
        <translation type="unfinished">Громкость</translation>
    </message>
    <message>
        <source>Volume:</source>
        <translation type="unfinished">Громкость:</translation>
    </message>
    <message>
        <source>VOL</source>
        <translation type="unfinished">ГРМК</translation>
    </message>
    <message>
        <source>Panning</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Panning:</source>
        <translation type="unfinished">Стереобаланс:</translation>
    </message>
    <message>
        <source>PAN</source>
        <translation type="unfinished">БАЛ</translation>
    </message>
    <message>
        <source>MIDI</source>
        <translation type="unfinished">MIDI</translation>
    </message>
    <message>
        <source>Input</source>
        <translation type="unfinished">Вход</translation>
    </message>
    <message>
        <source>Output</source>
        <translation type="unfinished">Выход</translation>
    </message>
</context>
<context>
    <name>InstrumentTrackWindow</name>
    <message>
        <source>GENERAL SETTINGS</source>
        <translation type="unfinished">ОБЩИЕ ПАРАМЕТРЫ</translation>
    </message>
    <message>
        <source>Click here, if you want to save current channel settings in a preset-file. Later you can load this preset by double-clicking it in the preset-browser.</source>
        <translation type="unfinished">Нажмите сюда, чтобы сохранить настройки текущего канала в файл предустановок. Позже вы сможете загрузить эти настройки через просмотрщик.</translation>
    </message>
    <message>
        <source>Instrument volume</source>
        <translation type="unfinished">Громкость инструмента</translation>
    </message>
    <message>
        <source>Volume:</source>
        <translation type="unfinished">Громкость:</translation>
    </message>
    <message>
        <source>VOL</source>
        <translation type="unfinished">ГРМК</translation>
    </message>
    <message>
        <source>Panning</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Panning:</source>
        <translation type="unfinished">Стереобаланс:</translation>
    </message>
    <message>
        <source>PAN</source>
        <translation type="unfinished">БАЛ</translation>
    </message>
    <message>
        <source>Pitch</source>
        <translation type="unfinished">Сдвиг высоты</translation>
    </message>
    <message>
        <source>Pitch:</source>
        <translation type="unfinished">Сдвиг высоты:</translation>
    </message>
    <message>
        <source>cents</source>
        <translation type="unfinished">центов</translation>
    </message>
    <message>
        <source>PITCH</source>
        <translation type="unfinished">СДВИГ</translation>
    </message>
    <message>
        <source>FX channel</source>
        <translation type="unfinished">Канал FX</translation>
    </message>
    <message>
        <source>ENV/LFO</source>
        <translation type="unfinished">ОБЛ/LFO</translation>
    </message>
    <message>
        <source>FUNC</source>
        <translation type="unfinished">АКК / АРП</translation>
    </message>
    <message>
        <source>FX</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>MIDI</source>
        <translation type="unfinished">MIDI</translation>
    </message>
    <message>
        <source>Save preset</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>XML preset file (*.xpf)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>PLUGIN</source>
        <translation type="unfinished">МОДУЛЬ</translation>
    </message>
    <message>
        <source>Save current channel settings in a preset-file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Pitch range (semitones)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>RANGE</source>
        <translation type="unfinished">ДИАПАЗОН</translation>
    </message>
</context>
<context>
    <name>LadspaControl</name>
    <message>
        <source>Link channels</source>
        <translation type="unfinished">Связать каналы</translation>
    </message>
</context>
<context>
    <name>LadspaControlDialog</name>
    <message>
        <source>Link Channels</source>
        <translation type="unfinished">Связать каналы</translation>
    </message>
    <message>
        <source>Channel </source>
        <translation type="unfinished">Канал </translation>
    </message>
</context>
<context>
    <name>LadspaControlView</name>
    <message>
        <source>Link channels</source>
        <translation type="unfinished">Связать каналы</translation>
    </message>
    <message>
        <source>Value:</source>
        <translation type="unfinished">Значение: </translation>
    </message>
    <message>
        <source>Sorry, no help available.</source>
        <translation type="unfinished">Извините, справки нет.</translation>
    </message>
</context>
<context>
    <name>LadspaEffect</name>
    <message>
        <source>Effect</source>
        <translation type="unfinished">Эффект</translation>
    </message>
    <message>
        <source>Unknown LADSPA plugin %1 requested.</source>
        <translation type="unfinished">Запрошен неизвестный модуль LADSPA «%1».</translation>
    </message>
</context>
<context>
    <name>LfoController</name>
    <message>
        <source>LFO Controller</source>
        <translation type="unfinished">Контроллер LFO</translation>
    </message>
    <message>
        <source>Base value</source>
        <translation type="unfinished">Опорное значение</translation>
    </message>
    <message>
        <source>Oscillator speed</source>
        <translation type="unfinished">Скорость генератора</translation>
    </message>
    <message>
        <source>Oscillator amount</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Oscillator phase</source>
        <translation type="unfinished">Фаза генератора</translation>
    </message>
    <message>
        <source>Oscillator waveform</source>
        <translation type="unfinished">Форма сигнала для генератора</translation>
    </message>
    <message>
        <source>Frequency Multiplier</source>
        <translation type="unfinished">Множитель частоты</translation>
    </message>
</context>
<context>
    <name>LfoControllerDialog</name>
    <message>
        <source>LFO</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>LFO Controller</source>
        <translation type="unfinished">Контроллер LFO</translation>
    </message>
    <message>
        <source>BASE</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Base amount:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>todo</source>
        <translation type="unfinished">сделать</translation>
    </message>
    <message>
        <source>SPD</source>
        <translation type="unfinished">СКОР</translation>
    </message>
    <message>
        <source>LFO-speed:</source>
        <translation type="unfinished">Скорость LFO:</translation>
    </message>
    <message>
        <source>Use this knob for setting speed of the LFO. The bigger this value the faster the LFO oscillates and the faster the effect.</source>
        <translation type="unfinished">Этот регулятор устанавлявает скорость LFO. Чем больше значение, тем больше частота генератора.</translation>
    </message>
    <message>
        <source>AMT</source>
        <translation type="unfinished">МОД</translation>
    </message>
    <message>
        <source>Modulation amount:</source>
        <translation type="unfinished">Глубина модуляции:</translation>
    </message>
    <message>
        <source>Use this knob for setting modulation amount of the LFO. The bigger this value, the more the connected control (e.g. volume or cutoff-frequency) will be influenced by the LFO.</source>
        <translation type="unfinished">Этот регулятор устанавливает глубину модуляции для LFO. Чем больше значение, тем в большей степени выбранная характеристика (н-р, громкость или частота среза) будет зависеть от LFO.</translation>
    </message>
    <message>
        <source>PHS</source>
        <translation type="unfinished">СДФ</translation>
    </message>
    <message>
        <source>Phase offset:</source>
        <translation type="unfinished">Сдвиг фазы:</translation>
    </message>
    <message>
        <source>degrees</source>
        <translation type="unfinished">˚</translation>
    </message>
    <message>
        <source>With this knob you can set the phase offset of the LFO. That means you can move the point within an oscillation where the oscillator begins to oscillate. For example if you have a sine-wave and have a phase-offset of 180 degrees the wave will first go down. It&apos;s the same with a square-wave.</source>
        <translation type="unfinished">Этот регулятор устанавливает начальную фазу генератора низкой чатсоты (LFO), т. е. точку, с которой генератор начинает вырабатывать сигнал. Например, если вы задали синусоидальную форму сигнала и начальную фазу 180º, волна сначала пойдёт вниз, а не вверх. То же для меандра (сигнала прямоугольной формы).</translation>
    </message>
    <message>
        <source>Click here for a sine-wave.</source>
        <translation type="unfinished">Генерировать гармонический (синусоидальный) сигнал.</translation>
    </message>
    <message>
        <source>Click here for a triangle-wave.</source>
        <translation type="unfinished">Сгенерировать треугольный сигнал.</translation>
    </message>
    <message>
        <source>Click here for a saw-wave.</source>
        <translation type="unfinished">Сгенерировать пилообразный сигнал.</translation>
    </message>
    <message>
        <source>Click here for a square-wave.</source>
        <translation type="unfinished">Сгенерировать меандр.</translation>
    </message>
    <message>
        <source>Click here for a a moog saw-wave.</source>
        <translation type="unfinished">Генерировать пилообразный сигнал как moog.</translation>
    </message>
    <message>
        <source>Click here for an exponential wave.</source>
        <translation type="unfinished">Генерировать экспоненциальный сигнал.</translation>
    </message>
    <message>
        <source>Click here for white-noise.</source>
        <translation type="unfinished">Сгенерировать белый шум.</translation>
    </message>
    <message>
        <source>Click here for a user-defined shape.
Double click to pick a file.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <source>Working directory</source>
        <translation type="unfinished">Рабочий каталог LMMS</translation>
    </message>
    <message>
        <source>The LMMS working directory %1 does not exist. Create it now? You can change the directory later via Edit -&gt; Settings.</source>
        <translation type="unfinished">Рабочий каталог LMMS (%1) не существует. Создать его? Позже вы сможете сменить его через Правка -&gt; Параметры.</translation>
    </message>
    <message>
        <source>Could not save config-file</source>
        <translation type="unfinished">Не могу сохранить настройки</translation>
    </message>
    <message>
        <source>Could not save configuration file %1. You&apos;re probably not permitted to write to this file.
Please make sure you have write-access to the file and try again.</source>
        <translation type="unfinished">Не могу записать настройки в файл %1. Возможно, вы не обладаете правами на запись в него.
Пожалуйста, проверьте свои права и попробуйте снова.</translation>
    </message>
    <message>
        <source>&amp;Project</source>
        <translation type="unfinished">&amp;Проект</translation>
    </message>
    <message>
        <source>&amp;New</source>
        <translation type="unfinished">&amp;Новый</translation>
    </message>
    <message>
        <source>&amp;Open...</source>
        <translation type="unfinished">&amp;Открыть...</translation>
    </message>
    <message>
        <source>Recently opened projects</source>
        <translation type="unfinished">Последние открывавшиеся проекты</translation>
    </message>
    <message>
        <source>&amp;Save</source>
        <translation type="unfinished">&amp;Сохранить</translation>
    </message>
    <message>
        <source>Save &amp;As...</source>
        <translation type="unfinished">Сохранить &amp;как...</translation>
    </message>
    <message>
        <source>Import...</source>
        <translation type="unfinished">Импорт...</translation>
    </message>
    <message>
        <source>E&amp;xport...</source>
        <translation type="unfinished">&amp;Экспорт...</translation>
    </message>
    <message>
        <source>&amp;Quit</source>
        <translation type="unfinished">&amp;Выйти</translation>
    </message>
    <message>
        <source>&amp;Edit</source>
        <translation type="unfinished">&amp;Правка</translation>
    </message>
    <message>
        <source>Settings</source>
        <translation type="unfinished">Параметры</translation>
    </message>
    <message>
        <source>&amp;Tools</source>
        <translation type="unfinished">&amp;Сервис</translation>
    </message>
    <message>
        <source>&amp;Help</source>
        <translation type="unfinished">&amp;Справка</translation>
    </message>
    <message>
        <source>Online help</source>
        <translation type="unfinished">Справка в сети</translation>
    </message>
    <message>
        <source>Help</source>
        <translation type="unfinished">Справка</translation>
    </message>
    <message>
        <source>What&apos;s this?</source>
        <translation type="unfinished">Что это?</translation>
    </message>
    <message>
        <source>About</source>
        <translation type="unfinished">О программе</translation>
    </message>
    <message>
        <source>Create new project</source>
        <translation type="unfinished">Создать новый проект</translation>
    </message>
    <message>
        <source>Create new project from template</source>
        <translation type="unfinished">Создать новый проект по шаблону</translation>
    </message>
    <message>
        <source>Open existing project</source>
        <translation type="unfinished">Открыть существующий проект</translation>
    </message>
    <message>
        <source>Recently opened project</source>
        <translation type="unfinished">Последние открывавшиеся проекты</translation>
    </message>
    <message>
        <source>Save current project</source>
        <translation type="unfinished">Сохранить текущий проект</translation>
    </message>
    <message>
        <source>Export current project</source>
        <translation type="unfinished">Экспортировать проект в звуковой файл</translation>
    </message>
    <message>
        <source>Show/hide Song-Editor</source>
        <translation type="unfinished">Показать/скрыть финальный секвенсор</translation>
    </message>
    <message>
        <source>By pressing this button, you can show or hide the Song-Editor. With the help of the Song-Editor you can edit song-playlist and specify when which track should be played. You can also insert and move samples (e.g. rap samples) directly into the playlist.</source>
        <translation type="unfinished">По нажатию этой кнопки запускается финальный секвенсор. С его помощью вы можете редактировать композицию и устанавливать время воспроизведения для всех дорожек. Также вы можете вставлять и передвигать образцы непосредственно в списке воспроизведения.</translation>
    </message>
    <message>
        <source>Show/hide Beat+Bassline Editor</source>
        <translation type="unfinished">Показать/скрыть пошаговый секвенсор</translation>
    </message>
    <message>
        <source>By pressing this button, you can show or hide the Beat+Bassline Editor. The Beat+Bassline Editor is needed for creating beats, and for opening, adding, and removing channels, and for cutting, copying and pasting beat and bassline-patterns, and for other things like that.</source>
        <translation type="unfinished">По нажатию этой кнопки запускается пошаговый секвенсор произведения. Он необходим для установки ритма, добавления и удаления каналов, вырезания, копирования и вставки шаблонов и т. п.</translation>
    </message>
    <message>
        <source>Show/hide Piano-Roll</source>
        <translation type="unfinished">Показать/скрыть нотный секвенсор</translation>
    </message>
    <message>
        <source>Click here to show or hide the Piano-Roll. With the help of the Piano-Roll you can edit melodies in an easy way.</source>
        <translation type="unfinished">По нажатию этой кнопки запускается секвенсор LMMS. С его помощью вы можете легко редактировать шаблоны мелодии.</translation>
    </message>
    <message>
        <source>Show/hide Automation Editor</source>
        <translation type="unfinished">Показать/скрыть редактор временн&lt;b&gt;ы&lt;/b&gt;х зависимостей</translation>
    </message>
    <message>
        <source>Click here to show or hide the Automation Editor. With the help of the Automation Editor you can edit dynamic values in an easy way.</source>
        <translation type="unfinished">Нажатием этой кнопки показывается/убирается окно редактора временн&lt;b&gt;ы&lt;/b&gt;х зависимостей. С его помощью вы можете редактировать изменения выбранных величин со временем.</translation>
    </message>
    <message>
        <source>Show/hide FX Mixer</source>
        <translation type="unfinished">Показать/скрыть микшер FX</translation>
    </message>
    <message>
        <source>Click here to show or hide the FX Mixer. The FX Mixer is a very powerful tool for managing effects for your song. You can insert effects into different effect-channels.</source>
        <translation type="unfinished">Нажмите здесь чтобы скрыть/показать микшер FX. Он является мощным инструментом для управления эффектами. Вы можете вставлять эффекты в различные каналы эффектов.</translation>
    </message>
    <message>
        <source>Show/hide project notes</source>
        <translation type="unfinished">Показать/скрыть заметки к проекту</translation>
    </message>
    <message>
        <source>Click here to show or hide the project notes window. In this window you can put down your project notes.</source>
        <translation type="unfinished">Эта кнопка показывает/прячет окно с заметками. В этом окне вы можете помещать любые комментарии к своей композиции.</translation>
    </message>
    <message>
        <source>Show/hide controller rack</source>
        <translation type="unfinished">Показать/скрыть управление контроллерами</translation>
    </message>
    <message>
        <source>Untitled</source>
        <translation type="unfinished">Неназванный</translation>
    </message>
    <message>
        <source>LMMS %1</source>
        <translation type="unfinished">LMMS %1</translation>
    </message>
    <message>
        <source>Project not saved</source>
        <translation type="unfinished">Проект не сохранён</translation>
    </message>
    <message>
        <source>The current project was modified since last saving. Do you want to save it now?</source>
        <translation type="unfinished">Проект был изменён. Сохранить его сейчас?</translation>
    </message>
    <message>
        <source>Open project</source>
        <translation type="unfinished">Открыть проект</translation>
    </message>
    <message>
        <source>Save project</source>
        <translation type="unfinished">Сохранить проект</translation>
    </message>
    <message>
        <source>Help not available</source>
        <translation type="unfinished">Справка недоступна</translation>
    </message>
    <message>
        <source>Currently there&apos;s no help available in LMMS.
Please visit http://lmms.sf.net/wiki for documentation on LMMS.</source>
        <translation type="unfinished">Пока что справка для LMMS не написана.
Вероятно, Вы сможете найти нужные материалы на http://lmms.sf.net/wiki .</translation>
    </message>
    <message>
        <source>My projects</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>My samples</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>My presets</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>My home</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>My computer</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Root directory</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Save as new &amp;version</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>E&amp;xport tracks...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>LMMS (*.mmp *.mmpz)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>LMMS Project (*.mmp *.mmpz);;LMMS Project Template (*.mpt)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Version %1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Project recovery</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>It looks like the last session did not end properly. Do you want to recover the project of this session?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Configuration file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Error while parsing configuration file at line %1:%2: %3</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>MeterDialog</name>
    <message>
        <source>Meter Numerator</source>
        <translation type="unfinished">Числитель</translation>
    </message>
    <message>
        <source>Meter Denominator</source>
        <translation type="unfinished">Знаменатель</translation>
    </message>
    <message>
        <source>TIME SIG</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>MeterModel</name>
    <message>
        <source>Numerator</source>
        <translation type="unfinished">Числитель</translation>
    </message>
    <message>
        <source>Denominator</source>
        <translation type="unfinished">Знаменатель</translation>
    </message>
</context>
<context>
    <name>MidiAlsaRaw::setupWidget</name>
    <message>
        <source>DEVICE</source>
        <translation type="unfinished">УСТРОЙСТВО</translation>
    </message>
</context>
<context>
    <name>MidiAlsaSeq::setupWidget</name>
    <message>
        <source>DEVICE</source>
        <translation type="unfinished">УСТРОЙСТВО</translation>
    </message>
</context>
<context>
    <name>MidiController</name>
    <message>
        <source>MIDI Controller</source>
        <translation type="unfinished">Контроллер MIDI</translation>
    </message>
    <message>
        <source>unnamed_midi_controller</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>MidiImport</name>
    <message>
        <source>Setup incomplete</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>You do not have set up a default soundfont in the settings dialog (Edit-&gt;Settings). Therefore no sound will be played back after importing this MIDI file. You should download a General MIDI soundfont, specify it in settings dialog and try again.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>You did not compile LMMS with support for SoundFont2 player, which is used to add default sound to imported MIDI files. Therefore no sound will be played back after importing this MIDI file.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>MidiOss::setupWidget</name>
    <message>
        <source>DEVICE</source>
        <translation type="unfinished">УСТРОЙСТВО</translation>
    </message>
</context>
<context>
    <name>MidiPort</name>
    <message>
        <source>Input channel</source>
        <translation type="unfinished">Входной канал</translation>
    </message>
    <message>
        <source>Output channel</source>
        <translation type="unfinished">Выходной канал</translation>
    </message>
    <message>
        <source>Input controller</source>
        <translation type="unfinished">Входной контроллер</translation>
    </message>
    <message>
        <source>Output controller</source>
        <translation type="unfinished">Выходной контроллер</translation>
    </message>
    <message>
        <source>Fixed input velocity</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Fixed output velocity</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Output MIDI program</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Receive MIDI-events</source>
        <translation type="unfinished">Принимать события MIDI</translation>
    </message>
    <message>
        <source>Send MIDI-events</source>
        <translation type="unfinished">Отправлять события MIDI</translation>
    </message>
    <message>
        <source>Fixed output note</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>OscillatorObject</name>
    <message>
        <source>Osc %1 volume</source>
        <translation type="unfinished">Громкость генератора %1</translation>
    </message>
    <message>
        <source>Osc %1 panning</source>
        <translation type="unfinished">Стереобаланс для генератора %1</translation>
    </message>
    <message>
        <source>Osc %1 coarse detuning</source>
        <translation type="unfinished">Расстройка генератора %1 (грубо)</translation>
    </message>
    <message>
        <source>Osc %1 fine detuning left</source>
        <translation type="unfinished">Расстройка левого канала генератора %1</translation>
    </message>
    <message>
        <source>Osc %1 fine detuning right</source>
        <translation type="unfinished">Расстройка правого канала генератора %1</translation>
    </message>
    <message>
        <source>Osc %1 phase-offset</source>
        <translation type="unfinished">Сдвиг фазы для генератора %1</translation>
    </message>
    <message>
        <source>Osc %1 stereo phase-detuning</source>
        <translation type="unfinished">Фазовая расстройка генератора %1 между каналами</translation>
    </message>
    <message>
        <source>Osc %1 wave shape</source>
        <translation type="unfinished">Форма сигнала генератора No %1</translation>
    </message>
    <message>
        <source>Modulation type %1</source>
        <translation type="unfinished">Тип модуляции: %1</translation>
    </message>
    <message>
        <source>Osc %1 waveform</source>
        <translation type="unfinished">Форма сигнала для генератора %1</translation>
    </message>
</context>
<context>
    <name>PatmanView</name>
    <message>
        <source>Open other patch</source>
        <translation type="unfinished">Открыть другой патч</translation>
    </message>
    <message>
        <source>Click here to open another patch-file. Loop and Tune settings are not reset.</source>
        <translation type="unfinished">Нажмите чтобы открыть другой патч-файл. Настройки при этом сохранятся.</translation>
    </message>
    <message>
        <source>Loop</source>
        <translation type="unfinished">Повторять</translation>
    </message>
    <message>
        <source>Loop mode</source>
        <translation type="unfinished">Режим повтора</translation>
    </message>
    <message>
        <source>Here you can toggle the Loop mode. If enabled, PatMan will use the loop information available in the file.</source>
        <translation type="unfinished">Здесь включается/выключается режим повтора. Если он включён, то PatMan будет использовать информацию о повторе из файла.</translation>
    </message>
    <message>
        <source>Tune</source>
        <translation type="unfinished">Подстроить</translation>
    </message>
    <message>
        <source>Tune mode</source>
        <translation type="unfinished">Подстроить под ноту</translation>
    </message>
    <message>
        <source>Here you can toggle the Tune mode. If enabled, PatMan will tune the sample to match the note&apos;s frequency.</source>
        <translation type="unfinished">Здесь включается/выключается режим подстройки. Если он включён, то PatMan изменит образец так, чтобы он совпадал по частоте с нотой.</translation>
    </message>
    <message>
        <source>No file selected</source>
        <translation type="unfinished">Не выбран файл</translation>
    </message>
    <message>
        <source>Open patch file</source>
        <translation type="unfinished">Открыть патч-файл</translation>
    </message>
    <message>
        <source>Patch-Files (*.pat)</source>
        <translation type="unfinished">Патч-файлы (*.pat)</translation>
    </message>
</context>
<context>
    <name>PeakController</name>
    <message>
        <source>Peak Controller</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Peak Controller Bug</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Due to a bug in older version of LMMS, the peak controllers may not be connect properly. Please ensure that peak controllers are connected properly and re-save this file. Sorry for any inconvenience caused.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>PeakControllerDialog</name>
    <message>
        <source>PEAK</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>LFO Controller</source>
        <translation type="unfinished">Контроллер LFO</translation>
    </message>
</context>
<context>
    <name>PeakControllerEffectControlDialog</name>
    <message>
        <source>BASE</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Base amount:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Modulation amount:</source>
        <translation type="unfinished">Глубина модуляции:</translation>
    </message>
    <message>
        <source>Attack:</source>
        <translation type="unfinished">Длительность нарастания:</translation>
    </message>
    <message>
        <source>Release:</source>
        <translation type="unfinished">Длительность исчезновения:</translation>
    </message>
    <message>
        <source>AMNT</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>MULT</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Amount Multiplicator:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>ATCK</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>DCAY</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>PeakControllerEffectControls</name>
    <message>
        <source>Base value</source>
        <translation type="unfinished">Опорное значение</translation>
    </message>
    <message>
        <source>Modulation amount</source>
        <translation type="unfinished">Глубина модуляции</translation>
    </message>
    <message>
        <source>Mute output</source>
        <translation type="unfinished">Заглушить</translation>
    </message>
    <message>
        <source>Attack</source>
        <translation type="unfinished">Длительность нарастания</translation>
    </message>
    <message>
        <source>Release</source>
        <translation type="unfinished">Длительность исчезновения</translation>
    </message>
    <message>
        <source>Abs Value</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Amount Multiplicator</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>PianoView</name>
    <message>
        <source>Base note</source>
        <translation type="unfinished">Опорная нота</translation>
    </message>
</context>
<context>
    <name>Plugin</name>
    <message>
        <source>Plugin not found</source>
        <translation type="unfinished">Модуль не найден</translation>
    </message>
    <message>
        <source>The plugin &quot;%1&quot; wasn&apos;t found or could not be loaded!
Reason: &quot;%2&quot;</source>
        <translation type="unfinished">Модуль «%1» отсутствует либо не может быть загружен!
Причина: «%2»</translation>
    </message>
    <message>
        <source>Error while loading plugin</source>
        <translation type="unfinished">Ошибка загрузки модуля</translation>
    </message>
    <message>
        <source>Failed to load plugin &quot;%1&quot;!</source>
        <translation type="unfinished">Ошибка загрузки модуля «%1»!</translation>
    </message>
</context>
<context>
    <name>ProjectRenderer</name>
    <message>
        <source>WAV-File (*.wav)</source>
        <translation type="unfinished">Файл WAV (*.wav)</translation>
    </message>
    <message>
        <source>Compressed OGG-File (*.ogg)</source>
        <translation type="unfinished">Сжатый файл OGG (*.ogg)</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <source>C</source>
        <comment>Note name</comment>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Db</source>
        <comment>Note name</comment>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>C#</source>
        <comment>Note name</comment>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>D</source>
        <comment>Note name</comment>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Eb</source>
        <comment>Note name</comment>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>D#</source>
        <comment>Note name</comment>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>E</source>
        <comment>Note name</comment>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Fb</source>
        <comment>Note name</comment>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Gb</source>
        <comment>Note name</comment>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>F#</source>
        <comment>Note name</comment>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>G</source>
        <comment>Note name</comment>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Ab</source>
        <comment>Note name</comment>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>G#</source>
        <comment>Note name</comment>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>A</source>
        <comment>Note name</comment>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Bb</source>
        <comment>Note name</comment>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>A#</source>
        <comment>Note name</comment>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>B</source>
        <comment>Note name</comment>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>QWidget</name>
    <message>
        <source>Name: </source>
        <translation>Название: </translation>
    </message>
    <message>
        <source>File: </source>
        <translation>Файл: </translation>
    </message>
    <message>
        <source>Maker: </source>
        <translation>Создал: </translation>
    </message>
    <message>
        <source>Copyright: </source>
        <translation>Права принадлежат: </translation>
    </message>
    <message>
        <source>Requires Real Time: </source>
        <translation>Требуется обработка в реальном времени:</translation>
    </message>
    <message>
        <source>Yes</source>
        <translation>Да</translation>
    </message>
    <message>
        <source>No</source>
        <translation>Нет</translation>
    </message>
    <message>
        <source>Real Time Capable: </source>
        <translation>Работа в реальном времени:</translation>
    </message>
    <message>
        <source>In Place Broken: </source>
        <translation>Вход и выход могут совпадать:</translation>
    </message>
    <message>
        <source>Channels In: </source>
        <translation>Входных каналов: </translation>
    </message>
    <message>
        <source>Channels Out: </source>
        <translation>Выходных каналов: </translation>
    </message>
</context>
<context>
    <name>SampleBuffer</name>
    <message>
        <source>Open audio file</source>
        <translation type="unfinished">Открыть звуковой файл</translation>
    </message>
    <message>
        <source>All Audio-Files (*.wav *.ogg *.ds *.flac *.spx *.voc *.aif *.aiff *.au *.raw *.mp3)</source>
        <translation type="unfinished">Все звуковые файлы (*.wav *.ogg *.ds *.flac *.spx *.voc *.aif *.aiff *.au *.raw *.mp3)</translation>
    </message>
    <message>
        <source>Wave-Files (*.wav)</source>
        <translation type="unfinished">Файлы Wave (*.wav)</translation>
    </message>
    <message>
        <source>OGG-Files (*.ogg)</source>
        <translation type="unfinished">Файлы OGG (*.ogg)</translation>
    </message>
    <message>
        <source>DrumSynth-Files (*.ds)</source>
        <translation type="unfinished">Файлы DrumSynth (*.ds)</translation>
    </message>
    <message>
        <source>FLAC-Files (*.flac)</source>
        <translation type="unfinished">Файлы FLAC (*.flac)</translation>
    </message>
    <message>
        <source>SPEEX-Files (*.spx)</source>
        <translation type="unfinished">Файлы SPEEX (*.spx)</translation>
    </message>
    <message>
        <source>MP3-Files (*.mp3)</source>
        <translation type="unfinished">Файлы MPEG1 L3 (*.mp3)</translation>
    </message>
    <message>
        <source>VOC-Files (*.voc)</source>
        <translation type="unfinished">Файлы VOC (*.voc)</translation>
    </message>
    <message>
        <source>AIFF-Files (*.aif *.aiff)</source>
        <translation type="unfinished">Файлы AIFF (*.aif *.aiff)</translation>
    </message>
    <message>
        <source>AU-Files (*.au)</source>
        <translation type="unfinished">Файлы AU (*.au)</translation>
    </message>
    <message>
        <source>RAW-Files (*.raw)</source>
        <translation type="unfinished">Файлы RAW (*.raw)</translation>
    </message>
</context>
<context>
    <name>SampleTCOView</name>
    <message>
        <source>double-click to select sample</source>
        <translation type="unfinished">Для выбора файла-образца сделайте двойной щелчок мышью</translation>
    </message>
    <message>
        <source>Delete (middle mousebutton)</source>
        <translation type="unfinished">Удалить (средняя кнопка мыши)</translation>
    </message>
    <message>
        <source>Cut</source>
        <translation type="unfinished">Вырезать</translation>
    </message>
    <message>
        <source>Copy</source>
        <translation type="unfinished">Копировать</translation>
    </message>
    <message>
        <source>Paste</source>
        <translation type="unfinished">Вставить</translation>
    </message>
    <message>
        <source>Mute/unmute (&lt;Ctrl&gt; + middle click)</source>
        <translation type="unfinished">Заглушить/включить (Crl + средняя кнопка мыши)</translation>
    </message>
    <message>
        <source>Set/clear record</source>
        <translation type="unfinished">Установить/очистить запись</translation>
    </message>
</context>
<context>
    <name>SampleTrack</name>
    <message>
        <source>Sample track</source>
        <translation type="unfinished">Дорожка</translation>
    </message>
    <message>
        <source>Volume</source>
        <translation type="unfinished">Громкость</translation>
    </message>
</context>
<context>
    <name>SampleTrackView</name>
    <message>
        <source>Track volume</source>
        <translation type="unfinished">Громкость дорожки</translation>
    </message>
    <message>
        <source>Channel volume:</source>
        <translation type="unfinished">Громкость канала:</translation>
    </message>
    <message>
        <source>VOL</source>
        <translation type="unfinished">ГРМК</translation>
    </message>
</context>
<context>
    <name>TempoSyncKnob</name>
    <message>
        <source>Tempo Sync</source>
        <translation type="unfinished">Синхронизация</translation>
    </message>
    <message>
        <source>No Sync</source>
        <translation type="unfinished">Синхронизации нет</translation>
    </message>
    <message>
        <source>Eight beats</source>
        <translation type="unfinished">Восемь ударов (две ноты)</translation>
    </message>
    <message>
        <source>Whole note</source>
        <translation type="unfinished">Целая нота</translation>
    </message>
    <message>
        <source>Half note</source>
        <translation type="unfinished">Полунота</translation>
    </message>
    <message>
        <source>Quarter note</source>
        <translation type="unfinished">Четверть ноты</translation>
    </message>
    <message>
        <source>8th note</source>
        <translation type="unfinished">Восьмая ноты</translation>
    </message>
    <message>
        <source>16th note</source>
        <translation type="unfinished">1/16 ноты</translation>
    </message>
    <message>
        <source>32nd note</source>
        <translation type="unfinished">1/32 ноты</translation>
    </message>
    <message>
        <source>Custom...</source>
        <translation type="unfinished">Настроить...</translation>
    </message>
    <message>
        <source>&amp;Help</source>
        <translation type="unfinished">&amp;Справка</translation>
    </message>
    <message>
        <source>Custom </source>
        <translation type="unfinished">Настроить </translation>
    </message>
    <message>
        <source>Synced to Eight Beats</source>
        <translation type="unfinished">Синхронизировано по двум нотам</translation>
    </message>
    <message>
        <source>Synced to Whole Note</source>
        <translation type="unfinished">Синхронизировано по целой ноте</translation>
    </message>
    <message>
        <source>Synced to Half Note</source>
        <translation type="unfinished">Синхронизировано по половине ноты</translation>
    </message>
    <message>
        <source>Synced to Quarter Note</source>
        <translation type="unfinished">Синхронизировано по четверти ноты</translation>
    </message>
    <message>
        <source>Synced to 8th Note</source>
        <translation type="unfinished">Синхронизировано по 1/8 ноты</translation>
    </message>
    <message>
        <source>Synced to 16th Note</source>
        <translation type="unfinished">Синхронизировано по 1/16 ноты</translation>
    </message>
    <message>
        <source>Synced to 32nd Note</source>
        <translation type="unfinished">Синхронизировано по 1/32 ноты</translation>
    </message>
</context>
<context>
    <name>TimeDisplayWidget</name>
    <message>
        <source>click to change time units</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>TrackContainer</name>
    <message>
        <source>Couldn&apos;t import file</source>
        <translation type="unfinished">Не могу импортировать файл</translation>
    </message>
    <message>
        <source>Couldn&apos;t find a filter for importing file %1.
You should convert this file into a format supported by LMMS using another software.</source>
        <translation type="unfinished">Не могу найти фильтр для файла  %1.
Для подключения этого файла преобразуйте его в формат, поддерживаемый LMMS.</translation>
    </message>
    <message>
        <source>Couldn&apos;t open file</source>
        <translation type="unfinished">Не могу открыть файл</translation>
    </message>
    <message>
        <source>Couldn&apos;t open file %1 for reading.
Please make sure you have read-permission to the file and the directory containing the file and try again!</source>
        <translation type="unfinished">Не могу открыть файл %1 для записи.
Проверьте, обладаете ли вы правами на запись в выбранный файл и содержащий его каталог и попробуйте снова!</translation>
    </message>
    <message>
        <source>Loading project...</source>
        <translation type="unfinished">Чтение проекта...</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation type="unfinished">Отменить</translation>
    </message>
    <message>
        <source>Please wait...</source>
        <translation type="unfinished">Подождите, пожалуйста...</translation>
    </message>
    <message>
        <source>Importing MIDI-file...</source>
        <translation type="unfinished">Импортирую файл MIDI...</translation>
    </message>
    <message>
        <source>Importing FLP-file...</source>
        <translation type="unfinished">Импортирую файл FLP...</translation>
    </message>
</context>
<context>
    <name>TripleOscillatorView</name>
    <message>
        <source>Use phase modulation for modulating oscillator 2 with oscillator 1</source>
        <translation type="unfinished">Модулировать фазу второго генератора сигналом с первого</translation>
    </message>
    <message>
        <source>Use amplitude modulation for modulating oscillator 2 with oscillator 1</source>
        <translation type="unfinished">Модулировать амплитуду генератора 2 сигналом с первого генератора</translation>
    </message>
    <message>
        <source>Mix output of oscillator 1 &amp; 2</source>
        <translation type="unfinished">&amp;Смешать сигналы первого и второго генераторов</translation>
    </message>
    <message>
        <source>Synchronize oscillator 1 with oscillator 2</source>
        <translation type="unfinished">Синхронизировать первый генератор по второму</translation>
    </message>
    <message>
        <source>Use frequency modulation for modulating oscillator 2 with oscillator 1</source>
        <translation type="unfinished">Модулировать частоту генератора 2 сигналом с первого генератора</translation>
    </message>
    <message>
        <source>Use phase modulation for modulating oscillator 3 with oscillator 2</source>
        <translation type="unfinished">Модулировать фазу третьего генератора сигналом со второго</translation>
    </message>
    <message>
        <source>Use amplitude modulation for modulating oscillator 3 with oscillator 2</source>
        <translation type="unfinished">Модулировать амплитуду генератора 3 сигналом со второго генератора</translation>
    </message>
    <message>
        <source>Mix output of oscillator 2 &amp; 3</source>
        <translation type="unfinished">&amp;Смешанный выход от второго и третьего генераторов</translation>
    </message>
    <message>
        <source>Synchronize oscillator 2 with oscillator 3</source>
        <translation type="unfinished">Синхронизировать второй генератор по третьему</translation>
    </message>
    <message>
        <source>Use frequency modulation for modulating oscillator 3 with oscillator 2</source>
        <translation type="unfinished">Модулировать частоту генератора 3 сигналом со второго генератора</translation>
    </message>
    <message>
        <source>Osc %1 volume:</source>
        <translation type="unfinished">Громкость генератора %1:</translation>
    </message>
    <message>
        <source>With this knob you can set the volume of oscillator %1. When setting a value of 0 the oscillator is turned off. Otherwise you can hear the oscillator as loud as you set it here.</source>
        <translation type="unfinished">Этот регулятор устанавливает громкость генератора %1. Если 0, то генератор выключается.</translation>
    </message>
    <message>
        <source>Osc %1 panning:</source>
        <translation type="unfinished">Стереобаланс для генератора %1:</translation>
    </message>
    <message>
        <source>With this knob you can set the panning of the oscillator %1. A value of -100 means 100% left and a value of 100 moves oscillator-output right.</source>
        <translation type="unfinished">Регулятор стереобаланса генератора %1. Величина -100 означает, что сигнал идёт только в левый канал, а 100 - в правый.</translation>
    </message>
    <message>
        <source>Osc %1 coarse detuning:</source>
        <translation type="unfinished">Расстройка генератора %1 (грубо):</translation>
    </message>
    <message>
        <source>semitones</source>
        <translation type="unfinished">полутон[а,ов]</translation>
    </message>
    <message>
        <source>With this knob you can set the coarse detuning of oscillator %1. You can detune the oscillator 12 semitones (1 octave) up and down. This is useful for creating sounds with a chord.</source>
        <translation type="unfinished">Грубая регулировка расстройки генератора %1. Возможна расстройка до 12 полутонов (до одной октавы) вверх и вниз. Полезно для создания аккордов.</translation>
    </message>
    <message>
        <source>Osc %1 fine detuning left:</source>
        <translation type="unfinished">Расстройка левого канала генератора %1:</translation>
    </message>
    <message>
        <source>cents</source>
        <translation type="unfinished">центов</translation>
    </message>
    <message>
        <source>With this knob you can set the fine detuning of oscillator %1 for the left channel. The fine-detuning is ranged between -100 cents and +100 cents. This is useful for creating &quot;fat&quot; sounds.</source>
        <translation type="unfinished">Этот регулятор устанавливает точную расстойку для левого канала генератора %1. Расстройка задаётся в диапазоне от -100 сотых до +100 сотых. Это полезно для создания &quot;толстых&quot; звуков.</translation>
    </message>
    <message>
        <source>Osc %1 fine detuning right:</source>
        <translation type="unfinished">Расстройка правого канала генератора %1:</translation>
    </message>
    <message>
        <source>With this knob you can set the fine detuning of oscillator %1 for the right channel. The fine-detuning is ranged between -100 cents and +100 cents. This is useful for creating &quot;fat&quot; sounds.</source>
        <translation type="unfinished">Этот регулятор устанавливает точную расстойку для правого канала генератора %1. Расстройка задаётся в диапазоне от -100 сотых до +100 сотых. Это полезно для создания &quot;толстых&quot; звуков.</translation>
    </message>
    <message>
        <source>Osc %1 phase-offset:</source>
        <translation type="unfinished">Сдвиг фазы для генератора %1:</translation>
    </message>
    <message>
        <source>degrees</source>
        <translation type="unfinished">˚</translation>
    </message>
    <message>
        <source>With this knob you can set the phase-offset of oscillator %1. That means you can move the point within an oscillation where the oscillator begins to oscillate. For example if you have a sine-wave and have a phase-offset of 180 degrees the wave will first go down. It&apos;s the same with a square-wave.</source>
        <translation type="unfinished">Этот регулятор устанавливает начальную фазу генератора №%1, т. е. точку, с которой генератор начинает вырабатывать сигнал. Например, если вы задали синусоидальную форму сигнала и начальную фазу 180º, волна сначала пойдёт вниз, а не вверх. То же для меандра (сигнала прямоугольной формы).</translation>
    </message>
    <message>
        <source>Osc %1 stereo phase-detuning:</source>
        <translation type="unfinished">Фазовая расстройка генератора %1 между каналами:</translation>
    </message>
    <message>
        <source>With this knob you can set the stereo phase-detuning of oscillator %1. The stereo phase-detuning specifies the size of the difference between the phase-offset of left and right channel. This is very good for creating wide stereo sounds.</source>
        <translation type="unfinished">Этот регулятор устанавливает фазовую расстройку генератора %1 между каналами, то есть разность фаз между левым и правым каналами. Это удобно для создания стереоэффектов.</translation>
    </message>
    <message>
        <source>Use a sine-wave for current oscillator.</source>
        <translation type="unfinished">Генерировать гармонический (синусоидальный) сигнал.</translation>
    </message>
    <message>
        <source>Use a triangle-wave for current oscillator.</source>
        <translation type="unfinished">Генерировать треугольный сигнал.</translation>
    </message>
    <message>
        <source>Use a saw-wave for current oscillator.</source>
        <translation type="unfinished">Генерировать пилообразный сигнал.</translation>
    </message>
    <message>
        <source>Use a square-wave for current oscillator.</source>
        <translation type="unfinished">Генерировать меандр.</translation>
    </message>
    <message>
        <source>Use a moog-like saw-wave for current oscillator.</source>
        <translation type="unfinished">Генерировать пилообразный сигнал, как в moog.</translation>
    </message>
    <message>
        <source>Use an exponential wave for current oscillator.</source>
        <translation type="unfinished">Генерировать экспоненциальный сигнал.</translation>
    </message>
    <message>
        <source>Use white-noise for current oscillator.</source>
        <translation type="unfinished">Генерировать белый шум.</translation>
    </message>
    <message>
        <source>Use a user-defined waveform for current oscillator.</source>
        <translation type="unfinished">Задать форму сигнала.</translation>
    </message>
</context>
<context>
    <name>Ui</name>
    <message>
        <source>Contributors ordered by number of commits:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Involved</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>VersionedSaveDialog</name>
    <message>
        <source>Increment version number</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Decrement version number</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>VestigeInstrumentView</name>
    <message>
        <source>Open other VST-plugin</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Click here, if you want to open another VST-plugin. After clicking on this button, a file-open-dialog appears and you can select your file.</source>
        <translation type="unfinished">Нажмите сюда, если вы хотите открыть другой модуль VST. После нажатия на кнопку появится стандартный диалог выбора файла, где вы сможете выбрать нужный модуль.</translation>
    </message>
    <message>
        <source>Show/hide GUI</source>
        <translation type="unfinished">Показать/скрыть интерфейс</translation>
    </message>
    <message>
        <source>Click here to show or hide the graphical user interface (GUI) of your VST-plugin.</source>
        <translation type="unfinished">Эта кнопка скрывает/показывает графический пользовательский интерфейс (GUI) выбранного модуля VST.</translation>
    </message>
    <message>
        <source>Turn off all notes</source>
        <translation type="unfinished">Выключить все ноты</translation>
    </message>
    <message>
        <source>Open VST-plugin</source>
        <translation type="unfinished">Открыть модуль VST</translation>
    </message>
    <message>
        <source>DLL-files (*.dll)</source>
        <translation type="unfinished">Бибилиотеки DLL (*.dll)</translation>
    </message>
    <message>
        <source>EXE-files (*.exe)</source>
        <translation type="unfinished">Программы EXE (*.exe)</translation>
    </message>
    <message>
        <source>No VST-plugin loaded</source>
        <translation type="unfinished">Модуль VST не загружен</translation>
    </message>
    <message>
        <source>Control VST-plugin from LMMS host</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Click here, if you want to control VST-plugin from host.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Open VST-plugin preset</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Click here, if you want to open another *.fxp, *.fxb VST-plugin preset.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Previous (-)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Click here, if you want to switch to another VST-plugin preset program.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Save preset</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Click here, if you want to save current VST-plugin preset program.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Next (+)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Click here to select presets that are currently loaded in VST.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Preset</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>by </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source> - VST plugin control</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>VstEffectControlDialog</name>
    <message>
        <source>Show/hide</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Control VST-plugin from LMMS host</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Click here, if you want to control VST-plugin from host.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Open VST-plugin preset</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Click here, if you want to open another *.fxp, *.fxb VST-plugin preset.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Previous (-)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Click here, if you want to switch to another VST-plugin preset program.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Next (+)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Click here to select presets that are currently loaded in VST.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Save preset</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Click here, if you want to save current VST-plugin preset program.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Effect by: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>&amp;nbsp;&amp;nbsp;&amp;nbsp;&amp;nbsp;&amp;nbsp;&amp;nbsp;&amp;nbsp;&lt;br /&gt;</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>VstPlugin</name>
    <message>
        <source>Loading plugin</source>
        <translation type="unfinished">Загрузка модуля</translation>
    </message>
    <message>
        <source>Please wait while loading VST-plugin...</source>
        <translation type="unfinished">Подождите, пока загружается модуль VST...</translation>
    </message>
    <message>
        <source>Failed loading VST-plugin</source>
        <translation type="unfinished">Не смог загрузить модуль VST</translation>
    </message>
    <message>
        <source>The VST-plugin %1 could not be loaded for some reason.
If it runs with other VST-software under Linux, please contact an LMMS-developer!</source>
        <translation type="unfinished">Отчего-то модуль VST %1 не мог быть загружен.
Если другое программное обеспечение VST работает у Вас под Linux&apos;ом, свяжитесь с разработчиком LMMS!</translation>
    </message>
    <message>
        <source>Open Preset</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Vst Plugin Preset (*.fxp *.fxb)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>: default</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>&quot;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>&apos;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Save Preset</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>.fxp</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>.FXP</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>.FXB</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>.fxb</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>ZynAddSubFxInstrument</name>
    <message>
        <source>Portamento</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Filter Frequency</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Filter Resonance</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Bandwidth</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>FM Gain</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Resonance Center Frequency</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Resonance Bandwidth</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Forward MIDI Control Change Events</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>ZynAddSubFxView</name>
    <message>
        <source>Show GUI</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Click here to show or hide the graphical user interface (GUI) of ZynAddSubFX.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Portamento:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>PORT</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Filter Frequency:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>FREQ</source>
        <translation type="unfinished">ЧАСТ</translation>
    </message>
    <message>
        <source>Filter Resonance:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>RES</source>
        <translation type="unfinished">УСИЛ</translation>
    </message>
    <message>
        <source>Bandwidth:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>BW</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>FM Gain:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>FM GAIN</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Resonance center frequency:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>RES CF</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Resonance bandwidth:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>RES BW</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Forward MIDI Control Changes</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>audioFileProcessor</name>
    <message>
        <source>Amplify</source>
        <translation>Усиление</translation>
    </message>
    <message>
        <source>Start of sample</source>
        <translation>Начало образца</translation>
    </message>
    <message>
        <source>End of sample</source>
        <translation>Конец образца</translation>
    </message>
    <message>
        <source>Reverse sample</source>
        <translation>Перевернуть образец</translation>
    </message>
    <message>
        <source>Loop</source>
        <translation>Повторять</translation>
    </message>
    <message>
        <source>Stutter</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>bassBoosterControlDialog</name>
    <message>
        <source>FREQ</source>
        <translation>ЧАСТ</translation>
    </message>
    <message>
        <source>Frequency:</source>
        <translation>Частота:</translation>
    </message>
    <message>
        <source>GAIN</source>
        <translation>УСИЛ</translation>
    </message>
    <message>
        <source>Gain:</source>
        <translation>Усиление:</translation>
    </message>
    <message>
        <source>RATIO</source>
        <translation>ОТН</translation>
    </message>
    <message>
        <source>Ratio:</source>
        <translation>Отношение:</translation>
    </message>
</context>
<context>
    <name>bassBoosterControls</name>
    <message>
        <source>Frequency</source>
        <translation>Частота</translation>
    </message>
    <message>
        <source>Gain</source>
        <translation>Усиление</translation>
    </message>
    <message>
        <source>Ratio</source>
        <translation>Отношение</translation>
    </message>
</context>
<context>
    <name>bbEditor</name>
    <message>
        <source>Play/pause current beat/bassline (Space)</source>
        <translation>Воспроизведение/пауза</translation>
    </message>
    <message>
        <source>Add beat/bassline</source>
        <translation>Добавить ритм/лейтмотив</translation>
    </message>
    <message>
        <source>Beat+Bassline Editor</source>
        <translation>Пошаговый секвенсор</translation>
    </message>
    <message>
        <source>Stop playback of current beat/bassline (Space)</source>
        <translation>Остановить воспроизведение текущего лейтмотива (ПРОБЕЛ)</translation>
    </message>
    <message>
        <source>Add automation-track</source>
        <translation>Добавить дорожку автоматизации</translation>
    </message>
    <message>
        <source>Click here to play the current beat/bassline.  The beat/bassline is automatically looped when its end is reached.</source>
        <translation>Нажмите чтобы проиграть запись лейтмтоива. Запись будет повторяться после окончания.</translation>
    </message>
    <message>
        <source>Click here to stop playing of current beat/bassline.</source>
        <translation>Остановить воспроизведение (Пробел).</translation>
    </message>
    <message>
        <source>Remove steps</source>
        <translation type="unfinished">Удалить шаги</translation>
    </message>
    <message>
        <source>Add steps</source>
        <translation type="unfinished">Добавить шаги</translation>
    </message>
</context>
<context>
    <name>bbTCOView</name>
    <message>
        <source>Open in Beat+Bassline-Editor</source>
        <translation>Открыть в пошаговом секвенсоре</translation>
    </message>
    <message>
        <source>Reset name</source>
        <translation>Сбросить название</translation>
    </message>
    <message>
        <source>Change name</source>
        <translation>Переименовать</translation>
    </message>
    <message>
        <source>Change color</source>
        <translation>Изменить цвет</translation>
    </message>
</context>
<context>
    <name>bbTrack</name>
    <message>
        <source>Beat/Bassline %1</source>
        <translation>Лейтмотив %1</translation>
    </message>
    <message>
        <source>Clone of %1</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>bitInvader</name>
    <message>
        <source>Samplelength</source>
        <translation>Длительность</translation>
    </message>
</context>
<context>
    <name>bitInvaderView</name>
    <message>
        <source>Sample Length</source>
        <translation>Длительность образца</translation>
    </message>
    <message>
        <source>Draw your own waveform here by dragging your mouse on this graph.</source>
        <translation>Здесь вы можете рисовать собственный сигнал.</translation>
    </message>
    <message>
        <source>Sine wave</source>
        <translation>Синусоида</translation>
    </message>
    <message>
        <source>Click for a sine-wave.</source>
        <translation>Сгенерировать гармонический (синусоидальный) сигнал.</translation>
    </message>
    <message>
        <source>Triangle wave</source>
        <translation>Треугольник</translation>
    </message>
    <message>
        <source>Click here for a triangle-wave.</source>
        <translation>Сгенерировать треугольный сигнал.</translation>
    </message>
    <message>
        <source>Saw wave</source>
        <translation>Пила</translation>
    </message>
    <message>
        <source>Click here for a saw-wave.</source>
        <translation>Сгенерировать пилообразный сигнал.</translation>
    </message>
    <message>
        <source>Square wave</source>
        <translation>Меандр</translation>
    </message>
    <message>
        <source>Click here for a square-wave.</source>
        <translation>Сгенерировать меандр.</translation>
    </message>
    <message>
        <source>White noise wave</source>
        <translation>Белый шум</translation>
    </message>
    <message>
        <source>Click here for white-noise.</source>
        <translation>Сгенерировать белый шум.</translation>
    </message>
    <message>
        <source>User defined wave</source>
        <translation>Пользовательская</translation>
    </message>
    <message>
        <source>Click here for a user-defined shape.</source>
        <translation>Задать форму сигнала самому.</translation>
    </message>
    <message>
        <source>Smooth</source>
        <translation>Сгладить</translation>
    </message>
    <message>
        <source>Click here to smooth waveform.</source>
        <translation>Щёлкните чтобы сгладить форму сигнала.</translation>
    </message>
    <message>
        <source>Interpolation</source>
        <translation>Интерполяция</translation>
    </message>
    <message>
        <source>Normalize</source>
        <translation>Нормализовать</translation>
    </message>
</context>
<context>
    <name>exportProjectDialog</name>
    <message>
        <source>Could not open file</source>
        <translation type="unfinished">Не могу открыть файл</translation>
    </message>
    <message>
        <source>Could not open file %1 for writing.
Please make sure you have write-permission to the file and the directory containing the file and try again!</source>
        <translation type="unfinished">Не могу открыть файл %1 для записи.
Проверьте, обладаете ли вы правами на запись в выбранный файл и содержащий его каталог и попробуйте снова!</translation>
    </message>
    <message>
        <source>Error</source>
        <translation type="unfinished">Ошибка</translation>
    </message>
    <message>
        <source>Error while determining file-encoder device. Please try to choose a different output format.</source>
        <translation type="unfinished">Ошибка при определении кодировщика файла. Попробуйте выбрать другой целевой формат.</translation>
    </message>
    <message>
        <source>Rendering: %1%</source>
        <translation type="unfinished">Обработка: %1%</translation>
    </message>
    <message>
        <source>Export project to %1</source>
        <translation type="unfinished">Экспорт композиции в файл %1</translation>
    </message>
</context>
<context>
    <name>fader</name>
    <message>
        <source>Please enter a new value between %1 and %2:</source>
        <translation type="unfinished">Введите новое значение между %1 и %2:</translation>
    </message>
</context>
<context>
    <name>fileBrowser</name>
    <message>
        <source>Browser</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>fileBrowserTreeWidget</name>
    <message>
        <source>Send to active instrument-track</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Open in new instrument-track/Song-Editor</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Open in new instrument-track/B+B Editor</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Loading sample</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Please wait, loading sample for preview...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>--- Factory files ---</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>graphModel</name>
    <message>
        <source>Graph</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>kickerInstrument</name>
    <message>
        <source>Start frequency</source>
        <translation>Начальная частота</translation>
    </message>
    <message>
        <source>End frequency</source>
        <translation>Конечная частота</translation>
    </message>
    <message>
        <source>Decay</source>
        <translation>Длит. спада</translation>
    </message>
    <message>
        <source>Distortion</source>
        <translation>Искажение</translation>
    </message>
    <message>
        <source>Gain</source>
        <translation>Усиление</translation>
    </message>
</context>
<context>
    <name>kickerInstrumentView</name>
    <message>
        <source>Start frequency:</source>
        <translation>Начальная частота:</translation>
    </message>
    <message>
        <source>End frequency:</source>
        <translation>Конечная частота:</translation>
    </message>
    <message>
        <source>Decay:</source>
        <translation>Длительность спада:</translation>
    </message>
    <message>
        <source>Distortion:</source>
        <translation>Искажение:</translation>
    </message>
    <message>
        <source>Gain:</source>
        <translation>Усиление:</translation>
    </message>
</context>
<context>
    <name>knob</name>
    <message>
        <source>&amp;Help</source>
        <translation>&amp;Справка</translation>
    </message>
    <message>
        <source>Please enter a new value between %1 and %2:</source>
        <translation>Введите новое значение между %1 и %2:</translation>
    </message>
    <message>
        <source>Please enter a new value between -96.0 dBV and 6.0 dBV:</source>
        <translation>Введите новое значение между –96,0 дБ и 6,0 дБ (по напряжению):</translation>
    </message>
</context>
<context>
    <name>ladspaBrowserView</name>
    <message>
        <source>Available Effects</source>
        <translation>Доступные эффекты</translation>
    </message>
    <message>
        <source>Unavailable Effects</source>
        <translation>Недоступные эффекты</translation>
    </message>
    <message>
        <source>Instruments</source>
        <translation>Инструменты</translation>
    </message>
    <message>
        <source>Analysis Tools</source>
        <translation>Анализаторы</translation>
    </message>
    <message>
        <source>Don&apos;t know</source>
        <translation>Неизвестные</translation>
    </message>
    <message>
        <source>This dialog displays information on all of the LADSPA plugins LMMS was able to locate. The plugins are divided into five categories based upon an interpretation of the port types and names.

Available Effects are those that can be used by LMMS. In order for LMMS to be able to use an effect, it must, first and foremost, be an effect, which is to say, it has to have both input channels and output channels. LMMS identifies an input channel as an audio rate port containing &apos;in&apos; in the name. Output channels are identified by the letters &apos;out&apos;. Furthermore, the effect must have the same number of inputs and outputs and be real time capable.

Unavailable Effects are those that were identified as effects, but either didn&apos;t have the same number of input and output channels or weren&apos;t real time capable.

Instruments are plugins for which only output channels were identified.

Analysis Tools are plugins for which only input channels were identified.

Don&apos;t Knows are plugins for which no input or output channels were identified.

Double clicking any of the plugins will bring up information on the ports.</source>
        <translation>В этом окне показана информация обо всех модулях LADSPA, которые обнаружила LMMS. Они разделены на пять категорий, в зависимости от названий и типов портов.

Доступные эффекты — это те, которые могут быть использоаны в LMMS. Чтобы эффект LADSPA мог быть использован, он должен, во-первых, быть собственно эффектом, т. е. иметь как входные так и выходные каналы. LMMS в качестве входного канала воспринимает аудиопорт, содержащий в названии „in“, а выходные узнаёт по подстроке „out“. Для использования в LMMS число входных каналов должно совпадать с числом выходных, и эффект должен иметь возможность использования в реальном времени.

Недоступные эффекты — это модули LADSPA, опознанные в качестве эффектов, однако либо с несовпадающими количестами входных/выходных каналов, либо не предназначенные для использования в реальном времени.

Инструменты — это модули, у которых есть только выходные каналы.

Анализаторы — это модули, обладающие лишь входными каналами.

Неизвестные — модули, у которых не было обнаружено ни входных, ни выходных каналов.

Двойной щелчок лувой кнопкой мыши по модулю даст информацию о его портах.</translation>
    </message>
    <message>
        <source>Type:</source>
        <translation>Тип:</translation>
    </message>
</context>
<context>
    <name>ladspaDescription</name>
    <message>
        <source>Plugins</source>
        <translation>Модули</translation>
    </message>
    <message>
        <source>Description</source>
        <translation>Описание</translation>
    </message>
</context>
<context>
    <name>ladspaPortDialog</name>
    <message>
        <source>Name</source>
        <translation>Название</translation>
    </message>
    <message>
        <source>Rate</source>
        <translation>Частота выборки</translation>
    </message>
    <message>
        <source>Direction</source>
        <translation>Направление</translation>
    </message>
    <message>
        <source>Type</source>
        <translation>Тип</translation>
    </message>
    <message>
        <source>Min &lt; Default &lt; Max</source>
        <translation>Наим. &lt; Стандарт &lt; Наиб.</translation>
    </message>
    <message>
        <source>Logarithmic</source>
        <translation>Логарифмический</translation>
    </message>
    <message>
        <source>SR Dependent</source>
        <translation></translation>
    </message>
    <message>
        <source>Audio</source>
        <translation>Аудио</translation>
    </message>
    <message>
        <source>Control</source>
        <translation>Управление</translation>
    </message>
    <message>
        <source>Input</source>
        <translation>Вход</translation>
    </message>
    <message>
        <source>Output</source>
        <translation>Выход</translation>
    </message>
    <message>
        <source>Toggled</source>
        <translation>Включено</translation>
    </message>
    <message>
        <source>Integer</source>
        <translation>Целое</translation>
    </message>
    <message>
        <source>Float</source>
        <translation>Дробное</translation>
    </message>
    <message>
        <source>Yes</source>
        <translation>Да</translation>
    </message>
    <message>
        <source>Ports</source>
        <translation>Порты</translation>
    </message>
</context>
<context>
    <name>lb302Synth</name>
    <message>
        <source>VCF Cutoff Frequency</source>
        <translation>Частота среза VCF</translation>
    </message>
    <message>
        <source>VCF Resonance</source>
        <translation>Усиление</translation>
    </message>
    <message>
        <source>VCF Envelope Mod</source>
        <translation>Глубина модуляции VCF</translation>
    </message>
    <message>
        <source>VCF Envelope Decay</source>
        <translation>Спад VCF</translation>
    </message>
    <message>
        <source>Slide</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Accent</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Dead</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Slide Decay</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Distortion</source>
        <translation>Искажение</translation>
    </message>
    <message>
        <source>Waveform</source>
        <translation>Форма сигнала</translation>
    </message>
    <message>
        <source>24dB/oct Filter</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>lb302SynthView</name>
    <message>
        <source>Cutoff Freq:</source>
        <translation>Частота среза:</translation>
    </message>
    <message>
        <source>Resonance:</source>
        <translation>Усиление:</translation>
    </message>
    <message>
        <source>Env Mod:</source>
        <translation>Глубина модуляции:</translation>
    </message>
    <message>
        <source>Decay:</source>
        <translation>Длительность спада:</translation>
    </message>
    <message>
        <source>303-es-que, 24dB/octave, 3 pole filter</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Slide Decay:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>DIST:</source>
        <translation>ИСК:</translation>
    </message>
    <message>
        <source>Saw wave</source>
        <translation type="unfinished">Пила</translation>
    </message>
    <message>
        <source>Click here for a saw-wave.</source>
        <translation type="unfinished">Сгенерировать пилообразный сигнал.</translation>
    </message>
    <message>
        <source>Triangle wave</source>
        <translation type="unfinished">Треугольник</translation>
    </message>
    <message>
        <source>Click here for a triangle-wave.</source>
        <translation type="unfinished">Сгенерировать треугольный сигнал.</translation>
    </message>
    <message>
        <source>Square wave</source>
        <translation type="unfinished">Меандр</translation>
    </message>
    <message>
        <source>Click here for a square-wave.</source>
        <translation type="unfinished">Сгенерировать меандр.</translation>
    </message>
    <message>
        <source>Rounded square wave</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Click here for a square-wave with a rounded end.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Moog wave</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Click here for a moog-like wave.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Sine wave</source>
        <translation type="unfinished">Синусоида</translation>
    </message>
    <message>
        <source>Click for a sine-wave.</source>
        <translation type="unfinished">Сгенерировать гармонический (синусоидальный) сигнал.</translation>
    </message>
    <message>
        <source>White noise wave</source>
        <translation type="unfinished">Белый шум</translation>
    </message>
    <message>
        <source>Click here for an exponential wave.</source>
        <translation type="unfinished">Генерировать экспоненциальный сигнал.</translation>
    </message>
    <message>
        <source>Click here for white-noise.</source>
        <translation type="unfinished">Сгенерировать белый шум.</translation>
    </message>
</context>
<context>
    <name>lb303Synth</name>
    <message>
        <source>VCF Cutoff Frequency</source>
        <translation>Частота среза VCF</translation>
    </message>
    <message>
        <source>VCF Resonance</source>
        <translation>Усиление</translation>
    </message>
    <message>
        <source>VCF Envelope Mod</source>
        <translation>Глубина модуляции VCF</translation>
    </message>
    <message>
        <source>VCF Envelope Decay</source>
        <translation>Спад VCF</translation>
    </message>
    <message>
        <source>Distortion</source>
        <translation>Искажение</translation>
    </message>
    <message>
        <source>Waveform</source>
        <translation>Форма сигнала</translation>
    </message>
    <message>
        <source>Slide Decay</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Slide</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Accent</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Dead</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>24dB/oct Filter</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>lb303SynthView</name>
    <message>
        <source>Cutoff Freq:</source>
        <translation>Частота среза:</translation>
    </message>
    <message>
        <source>CUT</source>
        <translation>СРЕЗ</translation>
    </message>
    <message>
        <source>Resonance:</source>
        <translation>Усиление:</translation>
    </message>
    <message>
        <source>RES</source>
        <translation>УСИЛ</translation>
    </message>
    <message>
        <source>Env Mod:</source>
        <translation>Глубина модуляции:</translation>
    </message>
    <message>
        <source>ENV MOD</source>
        <translation>МОД</translation>
    </message>
    <message>
        <source>Decay:</source>
        <translation>Длительность спада:</translation>
    </message>
    <message>
        <source>DEC</source>
        <translation>СПАД</translation>
    </message>
    <message>
        <source>303-es-que, 24dB/octave, 3 pole filter</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Slide Decay:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>SLIDE</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>DIST:</source>
        <translation>ИСК:</translation>
    </message>
    <message>
        <source>DIST</source>
        <translation>ИСК</translation>
    </message>
    <message>
        <source>WAVE:</source>
        <translation>Форма:</translation>
    </message>
    <message>
        <source>WAVE</source>
        <translation>Форма</translation>
    </message>
</context>
<context>
    <name>malletsInstrument</name>
    <message>
        <source>Hardness</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Position</source>
        <translation>Положение</translation>
    </message>
    <message>
        <source>Vibrato Gain</source>
        <translation>Усиление вибрато</translation>
    </message>
    <message>
        <source>Vibrato Freq</source>
        <translation>Частота вибрато</translation>
    </message>
    <message>
        <source>Stick Mix</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Modulator</source>
        <translation>Модулятор</translation>
    </message>
    <message>
        <source>Crossfade</source>
        <translation>Переход</translation>
    </message>
    <message>
        <source>LFO Speed</source>
        <translation>Скорость LFO</translation>
    </message>
    <message>
        <source>LFO Depth</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>ADSR</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Pressure</source>
        <translation>Давление</translation>
    </message>
    <message>
        <source>Motion</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Speed</source>
        <translation>Скорость</translation>
    </message>
    <message>
        <source>Bowed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Spread</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Missing files</source>
        <translation>Отсутствующие файлы</translation>
    </message>
    <message>
        <source>Your Stk-installation seems to be incomplete. Please make sure the full Stk-package is installed!</source>
        <translation>Похоже, что установлены не все пакеты STK. Вам следует это проверить!</translation>
    </message>
    <message>
        <source>Marimba</source>
        <translation>Маримба</translation>
    </message>
    <message>
        <source>Vibraphone</source>
        <translation>Вибрафон</translation>
    </message>
    <message>
        <source>Agogo</source>
        <translation>Дискотека</translation>
    </message>
    <message>
        <source>Wood1</source>
        <translation>Дерево1</translation>
    </message>
    <message>
        <source>Reso</source>
        <translation>УСИЛ</translation>
    </message>
    <message>
        <source>Wood2</source>
        <translation>Дерево2</translation>
    </message>
    <message>
        <source>Beats</source>
        <translation>Удары</translation>
    </message>
    <message>
        <source>Two Fixed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Clump</source>
        <translation>Тяжёлая поступь</translation>
    </message>
    <message>
        <source>Tubular Bells</source>
        <translation>Трубчатые колокольчики</translation>
    </message>
    <message>
        <source>Uniform Bar</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Tuned Bar</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Glass</source>
        <translation>Стекло</translation>
    </message>
    <message>
        <source>Tibetan Bowl</source>
        <translation>Тибетские шары</translation>
    </message>
</context>
<context>
    <name>malletsInstrumentView</name>
    <message>
        <source>Instrument</source>
        <translation>Инструмент</translation>
    </message>
    <message>
        <source>Spread</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Spread:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Hardness</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Hardness:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Position</source>
        <translation>Положение</translation>
    </message>
    <message>
        <source>Position:</source>
        <translation>Положение:</translation>
    </message>
    <message>
        <source>Vib Gain</source>
        <translation>Усил. вибрато</translation>
    </message>
    <message>
        <source>Vib Gain:</source>
        <translation>Усил. вибрато:</translation>
    </message>
    <message>
        <source>Vib Freq</source>
        <translation>Част. виб</translation>
    </message>
    <message>
        <source>Vib Freq:</source>
        <translation>Вибрато:</translation>
    </message>
    <message>
        <source>Stick Mix</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Stick Mix:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Modulator</source>
        <translation>Модулятор</translation>
    </message>
    <message>
        <source>Modulator:</source>
        <translation>Модулятор:</translation>
    </message>
    <message>
        <source>Crossfade</source>
        <translation>Переход</translation>
    </message>
    <message>
        <source>Crossfade:</source>
        <translation>Переход:</translation>
    </message>
    <message>
        <source>LFO Speed</source>
        <translation>Скорость LFO</translation>
    </message>
    <message>
        <source>LFO Speed:</source>
        <translation>Скорость LFO:</translation>
    </message>
    <message>
        <source>LFO Depth</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>LFO Depth:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>ADSR</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>ADSR:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Bowed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Pressure</source>
        <translation>Давление</translation>
    </message>
    <message>
        <source>Pressure:</source>
        <translation>Давление:</translation>
    </message>
    <message>
        <source>Motion</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Motion:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Speed</source>
        <translation>Скорость</translation>
    </message>
    <message>
        <source>Speed:</source>
        <translation>Скорость:</translation>
    </message>
    <message>
        <source>Vibrato</source>
        <translation>Вибрато</translation>
    </message>
    <message>
        <source>Vibrato:</source>
        <translation>Вибрато:</translation>
    </message>
</context>
<context>
    <name>manageVSTEffectView</name>
    <message>
        <source> - VST parameter control</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>VST Sync</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Click here if you want to synchronize all parameters with VST plugin.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Automated</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Click here if you want to display automated parameters only.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>    Close    </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Close VST effect knob-controller window.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>manageVestigeInstrumentView</name>
    <message>
        <source> - VST plugin control</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>VST Sync</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Click here if you want to synchronize all parameters with VST plugin.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Automated</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Click here if you want to display automated parameters only.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>    Close    </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Close VST plugin knob-controller window.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>nineButtonSelector</name>
    <message>
        <source>&amp;Help</source>
        <translation>&amp;Справка</translation>
    </message>
</context>
<context>
    <name>opl2instrument</name>
    <message>
        <source>Patch</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Op 1 Attack</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Op 1 Decay</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Op 1 Sustain</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Op 1 Release</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Op 1 Level</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Op 1 Level Scaling</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Op 1 Frequency Multiple</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Op 1 Feedback</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Op 1 Key Scaling Rate</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Op 1 Percussive Envelope</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Op 1 Tremolo</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Op 1 Vibrato</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Op 1 Waveform</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Op 2 Attack</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Op 2 Decay</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Op 2 Sustain</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Op 2 Release</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Op 2 Level</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Op 2 Level Scaling</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Op 2 Frequency Multiple</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Op 2 Key Scaling Rate</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Op 2 Percussive Envelope</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Op 2 Tremolo</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Op 2 Vibrato</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Op 2 Waveform</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>FM</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Vibrato Depth</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Tremolo Depth</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>organicInstrument</name>
    <message>
        <source>Distortion</source>
        <translation>Искажение</translation>
    </message>
    <message>
        <source>Volume</source>
        <translation>Громкость</translation>
    </message>
</context>
<context>
    <name>organicInstrumentView</name>
    <message>
        <source>Distortion:</source>
        <translation>Искажение:</translation>
    </message>
    <message>
        <source>Volume:</source>
        <translation>Громкость:</translation>
    </message>
    <message>
        <source>Randomise</source>
        <translation>Случайно</translation>
    </message>
    <message>
        <source>Osc %1 waveform:</source>
        <translation>Форма сигнала для генератора %1:</translation>
    </message>
    <message>
        <source>Osc %1 volume:</source>
        <translation>Громкость генератора %1:</translation>
    </message>
    <message>
        <source>Osc %1 panning:</source>
        <translation>Стереобаланс для генератора %1:</translation>
    </message>
    <message>
        <source>Osc %1 fine detuning left:</source>
        <translation>Расстройка левого канала генератора %1:</translation>
    </message>
    <message>
        <source>cents</source>
        <translation>центов</translation>
    </message>
</context>
<context>
    <name>papuInstrument</name>
    <message>
        <source>Sweep time</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Sweep direction</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Sweep RtShift amount</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Wave Pattern Duty</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Channel 1 volume</source>
        <translation>Громкость первого канала</translation>
    </message>
    <message>
        <source>Volume sweep direction</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Length of each step in sweep</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Channel 2 volume</source>
        <translation>Громкость второго канала</translation>
    </message>
    <message>
        <source>Channel 3 volume</source>
        <translation>Громкость третьего канала</translation>
    </message>
    <message>
        <source>Channel 4 volume</source>
        <translation>Громкость четвёртого канала</translation>
    </message>
    <message>
        <source>Right Output level</source>
        <translation>Выходной уровень справа</translation>
    </message>
    <message>
        <source>Left Output level</source>
        <translation>Выходной уровень слева</translation>
    </message>
    <message>
        <source>Channel 1 to SO2 (Left)</source>
        <translation>От первого канала к SO2 (левый канал)</translation>
    </message>
    <message>
        <source>Channel 2 to SO2 (Left)</source>
        <translation>От второго канала к SO2 (левый канал)</translation>
    </message>
    <message>
        <source>Channel 3 to SO2 (Left)</source>
        <translation>От третьего канала к SO2 (левый канал)</translation>
    </message>
    <message>
        <source>Channel 4 to SO2 (Left)</source>
        <translation>От четвёртого канала к SO2 (левый канал)</translation>
    </message>
    <message>
        <source>Channel 1 to SO1 (Right)</source>
        <translation>От первого канала к SO1 (правый канал)</translation>
    </message>
    <message>
        <source>Channel 2 to SO1 (Right)</source>
        <translation>От второго канала к SO1 (правый канал)</translation>
    </message>
    <message>
        <source>Channel 3 to SO1 (Right)</source>
        <translation>От третьего канала к SO1 (правый канал)</translation>
    </message>
    <message>
        <source>Channel 4 to SO1 (Right)</source>
        <translation>От четвёртого канала к SO1 (правый канал)</translation>
    </message>
    <message>
        <source>Treble</source>
        <translation>Верхние</translation>
    </message>
    <message>
        <source>Bass</source>
        <translation>Нижние</translation>
    </message>
    <message>
        <source>Shift Register width</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>papuInstrumentView</name>
    <message>
        <source>Sweep Time:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Sweep Time</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Sweep RtShift amount:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Sweep RtShift amount</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Wave pattern duty:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Wave Pattern Duty</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Square Channel 1 Volume:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Length of each step in sweep:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Length of each step in sweep</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Wave pattern duty</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Square Channel 2 Volume:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Square Channel 2 Volume</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Wave Channel Volume:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Wave Channel Volume</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Noise Channel Volume:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Noise Channel Volume</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>SO1 Volume (Right):</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>SO1 Volume (Right)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>SO2 Volume (Left):</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>SO2 Volume (Left)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Treble:</source>
        <translation>Верхние:</translation>
    </message>
    <message>
        <source>Treble</source>
        <translation>Верхние</translation>
    </message>
    <message>
        <source>Bass:</source>
        <translation>Нижние:</translation>
    </message>
    <message>
        <source>Bass</source>
        <translation>Нижние</translation>
    </message>
    <message>
        <source>Sweep Direction</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Volume Sweep Direction</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Shift Register Width</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Channel1 to SO1 (Right)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Channel2 to SO1 (Right)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Channel3 to SO1 (Right)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Channel4 to SO1 (Right)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Channel1 to SO2 (Left)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Channel2 to SO2 (Left)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Channel3 to SO2 (Left)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Channel4 to SO2 (Left)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Wave Pattern</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>The amount of increase or decrease in frequency</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>The rate at which increase or decrease in frequency occurs</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>The duty cycle is the ratio of the duration (time) that a signal is ON versus the total period of the signal.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Square Channel 1 Volume</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>The delay between step change</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Draw the wave here</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>pattern</name>
    <message>
        <source>Cannot freeze pattern</source>
        <translation>Не могу заморозить шаблон</translation>
    </message>
    <message>
        <source>The pattern currently cannot be freezed because you&apos;re in play-mode. Please stop and try again!</source>
        <translation>Шаблон не может быть заморожен, так как он проигрывается в данный момент. Остановите воспроизведение и попробуйте снова!</translation>
    </message>
</context>
<context>
    <name>patternFreezeStatusDialog</name>
    <message>
        <source>Freezing pattern...</source>
        <translation>Замораживаю шаблон...</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation>Отменить</translation>
    </message>
</context>
<context>
    <name>patternView</name>
    <message>
        <source>double-click to open this pattern in piano-roll
use mouse wheel to set volume of a step</source>
        <translation>Чтобы открыть этот шаблон в секвенсоре, дважды на нём щёлкните
Используйте колёсико мыши для установки громкости отдельного шага</translation>
    </message>
    <message>
        <source>Open in piano-roll</source>
        <translation>Открыть в секвенсоре</translation>
    </message>
    <message>
        <source>Clear all notes</source>
        <translation>Очистить</translation>
    </message>
    <message>
        <source>Reset name</source>
        <translation>Сбросить название</translation>
    </message>
    <message>
        <source>Change name</source>
        <translation>Переименовать</translation>
    </message>
    <message>
        <source>Refreeze</source>
        <translation>Перезаморозить</translation>
    </message>
    <message>
        <source>Freeze</source>
        <translation>Заморозить</translation>
    </message>
    <message>
        <source>Unfreeze</source>
        <translation>Разморозить</translation>
    </message>
    <message>
        <source>Add steps</source>
        <translation>Добавить шаги</translation>
    </message>
    <message>
        <source>Remove steps</source>
        <translation>Удалить шаги</translation>
    </message>
</context>
<context>
    <name>pianoRoll</name>
    <message>
        <source>Cut selected notes (Ctrl+X)</source>
        <translation>Переместить выделенные ноты в буфер (Ctrl+X)</translation>
    </message>
    <message>
        <source>Copy selected notes (Ctrl+C)</source>
        <translation>Копировать выделенные ноты в буфер (Ctrl+X)</translation>
    </message>
    <message>
        <source>Paste notes from clipboard (Ctrl+V)</source>
        <translation>Вставить ноты из буфера (Ctrl+V)</translation>
    </message>
    <message>
        <source>Play/pause current pattern (Space)</source>
        <translation>Воспроизведение ткущего шаблона/пауза (Пробел)</translation>
    </message>
    <message>
        <source>Stop playing of current pattern (Space)</source>
        <translation>Остановить воспроизвдение шаблона (Пробел)</translation>
    </message>
    <message>
        <source>Piano-Roll - no pattern</source>
        <translation>Секвенсор - без шаблона</translation>
    </message>
    <message>
        <source>Piano-Roll - %1</source>
        <translation>Секвенсор - %1</translation>
    </message>
    <message>
        <source>Please open a pattern by double-clicking on it!</source>
        <translation>Откройте шаблон с помощью двойного щелчка мышью!</translation>
    </message>
    <message>
        <source>Record notes from MIDI-device/channel-piano</source>
        <translation>Записать ноты с цифрового музыкального инструмента (MIDI)</translation>
    </message>
    <message>
        <source>Last note</source>
        <translation>По посл. ноте</translation>
    </message>
    <message>
        <source>Draw mode (Shift+D)</source>
        <translation>Режим рисования (Shift+D)</translation>
    </message>
    <message>
        <source>Erase mode (Shift+E)</source>
        <translation>Режим стирания нот (Shift+E)</translation>
    </message>
    <message>
        <source>Select mode (Shift+S)</source>
        <translation>Режим выбора нот (Shift+S)</translation>
    </message>
    <message>
        <source>Record notes from MIDI-device/channel-piano while playing song or BB track</source>
        <translation>Записать ноты с цифрового музыкального инструмента (MIDI) во время воспроизведения композиции или лейтмотива</translation>
    </message>
    <message>
        <source>Click here to play the current pattern. This is useful while editing it. The pattern is automatically looped when its end is reached.</source>
        <translation>Нажмите здесь чтобы проиграть текущий шаблон. Это может пригодиться при его редактировании. По окончании шаблона воспроизведение начнётся сначала.</translation>
    </message>
    <message>
        <source>Click here to record notes from a MIDI-device or the virtual test-piano of the according channel-window to the current pattern. When recording all notes you play will be written to this pattern and you can play and edit them afterwards.</source>
        <translation>Нажмите эту кнопку, если вы хотите записать ноты с устройства MIDI или виртуального синтезатора соответствующего канала. Позже вы сможете отредактировать записанный шаблон.</translation>
    </message>
    <message>
        <source>Click here to record notes from a MIDI-device or the virtual test-piano of the according channel-window to the current pattern. When recording all notes you play will be written to this pattern and you will hear the song or BB track in the background.</source>
        <translation>Нажмите эту кнопку, если вы хотите записать ноты с устройства MIDI или виртуального синтезатора соответствующего канала.  Во время записи все ноты записываются в этот шаблон, и вы будете слышать композицию или лейтмотив.</translation>
    </message>
    <message>
        <source>Click here to stop playback of current pattern.</source>
        <translation>Нажмите здесь, если вы хотите остановить воспроизведение текущего шаблона.</translation>
    </message>
    <message>
        <source>Click here and the selected notes will be cut into the clipboard. You can paste them anywhere in any pattern by clicking on the paste button.</source>
        <translation>При нажатии на эту кнопку выделеные ноты будут перемещены в буфер. Позже вы можете вставить их в любое место любого шаблона с помощью кнопки &quot;Вставить&quot;.</translation>
    </message>
    <message>
        <source>Click here and the selected notes will be copied into the clipboard. You can paste them anywhere in any pattern by clicking on the paste button.</source>
        <translation>При нажатии на эту кнопку выделеные ноты будут перемещены в буфер. Позже вы можете вставить их в любое место любого шаблона с помощью кнопки &quot;Вставить&quot;.</translation>
    </message>
    <message>
        <source>Click here and the notes from the clipboard will be pasted at the first visible measure.</source>
        <translation>При нажатии на эту кнопку ноты из буфера будут вставлены в превый видимый такт.</translation>
    </message>
    <message>
        <source>Note lock</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Note Volume</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Note Panning</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Detune mode (Shift+T)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Click here and draw mode will be activated. In this mode you can add, resize and move notes. This is the default mode which is used most of the time. You can also press &apos;Shift+D&apos; on your keyboard to activate this mode. In this mode, hold Ctrl to temporarily go into select mode.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Click here and erase mode will be activated. In this mode you can erase notes. You can also press &apos;Shift+E&apos; on your keyboard to activate this mode.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Click here and select mode will be activated. In this mode you can select notes. Alternatively, you can hold Ctrl in draw mode to temporarily use select mode.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Click here and detune mode will be activated. In this mode you can click a note to open its automation detuning. You can utilize this to slide notes from one to another. You can also press &apos;Shift+T&apos; on your keyboard to activate this mode.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Mark/unmark current semitone</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Mark current scale</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Mark current chord</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Unmark all</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>No scale</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>No chord</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>pluginBrowser</name>
    <message>
        <source>Instrument plugins</source>
        <translation>Инструменты</translation>
    </message>
    <message>
        <source>three powerful oscillators you can modulate in several ways</source>
        <translation>Три генератора, которые Вы можете настраивать по вкусу</translation>
    </message>
    <message>
        <source>no description</source>
        <translation>описание отсутствует</translation>
    </message>
    <message>
        <source>VST-host for using VST(i)-plugins within LMMS</source>
        <translation>Поддержка модулей VST(i) в LMMS</translation>
    </message>
    <message>
        <source>Additive Synthesizer for organ-like sounds</source>
        <translation>Синтезатор звуков вроде органа</translation>
    </message>
    <message>
        <source>simple sampler with various settings for using samples (e.g. drums) in an instrument-track</source>
        <translation>Подключение произвольного звукового образца с некоторой возможностью настройки</translation>
    </message>
    <message>
        <source>Filter for importing MIDI-files into LMMS</source>
        <translation>Фильтр для включение файла MIDI в проект LMMS</translation>
    </message>
    <message>
        <source>plugin for using arbitrary LADSPA-effects inside LMMS.</source>
        <translation>Модуль, позволяющий использовать в LMMS любые эффекты LADSPA.</translation>
    </message>
    <message>
        <source>Tuneful things to bang on</source>
        <translation>Мелодичные ударные</translation>
    </message>
    <message>
        <source>plugin for using arbitrary VST-effects inside LMMS.</source>
        <translation>Модуль, позволяющий использовать в LMMS любые эффекты VST.</translation>
    </message>
    <message>
        <source>Vibrating string modeler</source>
        <translation>Эмуляция струн</translation>
    </message>
    <message>
        <source>Filter for importing FL Studio projects into LMMS</source>
        <translation>Фильтр для импортирования файлов FL Stuio</translation>
    </message>
    <message>
        <source>Incomplete monophonic imitation tb303</source>
        <translation>Незавершённая монофоническая имитация tb303</translation>
    </message>
    <message>
        <source>versatile kick- &amp; bassdrum-synthesizer</source>
        <translation>Многоцелевой синтезатор ударных</translation>
    </message>
    <message>
        <source>Instrument browser</source>
        <translation>Инструменты</translation>
    </message>
    <message>
        <source>Drag an instrument into either the Song-Editor, the Beat+Bassline Editor or into an existing instrument track.</source>
        <translation>Вы можете переносить нужные вам инструменты из этой панели в финальный или пошаговый секвенсор.</translation>
    </message>
    <message>
        <source>Plugin for enhancing stereo separation of a stereo input file</source>
        <translation>Модуль, усиливающий разницу между каналами стереозаписи</translation>
    </message>
    <message>
        <source>plugin for boosting bass</source>
        <translation>Модуль, усиливающий басы</translation>
    </message>
    <message>
        <source>Emulation of GameBoy (TM) APU</source>
        <translation>Эмуляция аудиопроцессора GameBoy (TM)</translation>
    </message>
    <message>
        <source>Plugin for freely manipulating stereo output</source>
        <translation>Модуль для произвольного управления стереовыходом</translation>
    </message>
    <message>
        <source>Emulation of the MOS6581 and MOS8580 SID.
This chip was used in the Commodore 64 computer.</source>
        <translation>Эмуляция MOS6581 и MOS8580.
Использовалось на компьютере Commodore 64.</translation>
    </message>
    <message>
        <source>List installed LADSPA plugins</source>
        <translation>Показать установленные модули LADSPA</translation>
    </message>
    <message>
        <source>Player for SoundFont files</source>
        <translation>Проигрыватель файлов SoundFont</translation>
    </message>
    <message>
        <source>Plugin for controlling knobs with sound peaks</source>
        <translation>Модуль для установки значений регуляторов по пикам громкости</translation>
    </message>
    <message>
        <source>GUS-compatible patch instrument</source>
        <translation>Патч-инструмент, совместимый с GUS</translation>
    </message>
    <message>
        <source>Customizable wavetable synthesizer</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Embedded ZynAddSubFX</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>2-operator FM Synth</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Filter for importing Hydrogen files into LMMS</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>LMMS port of sfxr</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>projectNotes</name>
    <message>
        <source>Put down your project notes here.</source>
        <translation>Здесь вы можете держать заметки к своему проекту.</translation>
    </message>
    <message>
        <source>Project notes</source>
        <translation>Заметки к проекту</translation>
    </message>
    <message>
        <source>Edit Actions</source>
        <translation>Правка</translation>
    </message>
    <message>
        <source>&amp;Undo</source>
        <translation>&amp;Отменить</translation>
    </message>
    <message>
        <source>Ctrl+Z</source>
        <translation></translation>
    </message>
    <message>
        <source>&amp;Redo</source>
        <translation>&amp;Повторить</translation>
    </message>
    <message>
        <source>Ctrl+Y</source>
        <translation></translation>
    </message>
    <message>
        <source>&amp;Copy</source>
        <translation>&amp;Копировать</translation>
    </message>
    <message>
        <source>Ctrl+C</source>
        <translation></translation>
    </message>
    <message>
        <source>Cu&amp;t</source>
        <translation>&amp;Вырезать</translation>
    </message>
    <message>
        <source>Ctrl+X</source>
        <translation></translation>
    </message>
    <message>
        <source>&amp;Paste</source>
        <translation>&amp;Вставить</translation>
    </message>
    <message>
        <source>Ctrl+V</source>
        <translation></translation>
    </message>
    <message>
        <source>Format Actions</source>
        <translation>Форматирование</translation>
    </message>
    <message>
        <source>&amp;Bold</source>
        <translation>Полу&amp;жирный</translation>
    </message>
    <message>
        <source>Ctrl+B</source>
        <translation></translation>
    </message>
    <message>
        <source>&amp;Italic</source>
        <translation>&amp;Курсив</translation>
    </message>
    <message>
        <source>Ctrl+I</source>
        <translation></translation>
    </message>
    <message>
        <source>&amp;Underline</source>
        <translation>&amp;Подчеркнуть</translation>
    </message>
    <message>
        <source>Ctrl+U</source>
        <translation></translation>
    </message>
    <message>
        <source>&amp;Left</source>
        <translation>По &amp;левому краю</translation>
    </message>
    <message>
        <source>Ctrl+L</source>
        <translation></translation>
    </message>
    <message>
        <source>C&amp;enter</source>
        <translation>По &amp;центру</translation>
    </message>
    <message>
        <source>Ctrl+E</source>
        <translation></translation>
    </message>
    <message>
        <source>&amp;Right</source>
        <translation>По &amp;правому краю</translation>
    </message>
    <message>
        <source>Ctrl+R</source>
        <translation></translation>
    </message>
    <message>
        <source>&amp;Justify</source>
        <translation>По &amp;ширине</translation>
    </message>
    <message>
        <source>Ctrl+J</source>
        <translation></translation>
    </message>
    <message>
        <source>&amp;Color...</source>
        <translation>&amp;Цвет...</translation>
    </message>
</context>
<context>
    <name>renameDialog</name>
    <message>
        <source>Rename...</source>
        <translation>Переименовать...</translation>
    </message>
</context>
<context>
    <name>setupDialog</name>
    <message>
        <source>Setup LMMS</source>
        <translation>Настройка LMMS</translation>
    </message>
    <message>
        <source>General settings</source>
        <translation>Общие параметры</translation>
    </message>
    <message>
        <source>BUFFER SIZE</source>
        <translation>РАЗМЕР БУФЕРА</translation>
    </message>
    <message>
        <source>Reset to default-value</source>
        <translation>Восстановить значение по умолчанию</translation>
    </message>
    <message>
        <source>MISC</source>
        <translation>РАЗНОЕ</translation>
    </message>
    <message>
        <source>Audio settings</source>
        <translation>Параметры звука</translation>
    </message>
    <message>
        <source>AUDIO INTERFACE</source>
        <translation>ЗВУКОВАЯ СИСТЕМА</translation>
    </message>
    <message>
        <source>MIDI settings</source>
        <translation>Параметры MIDI</translation>
    </message>
    <message>
        <source>MIDI INTERFACE</source>
        <translation>ИНТЕРФЕЙС MIDI</translation>
    </message>
    <message>
        <source>OK</source>
        <translation></translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation>Отменить</translation>
    </message>
    <message>
        <source>Restart LMMS</source>
        <translation>Перезапустите LMMS</translation>
    </message>
    <message>
        <source>Please note that most changes won&apos;t take effect until you restart LMMS!</source>
        <translation>Учтите, что большинство настроек не вступят в силу до перезапуска программы!</translation>
    </message>
    <message>
        <source>Here you can setup the internal buffer-size used by LMMS. Smaller values result in a lower latency but also may cause unusable sound or bad performance, especially on older computers or systems with a non-realtime kernel.</source>
        <translation>Здесь вы можете настроить размер внутреннего звукового буфера LMMS. Меньшие значения дают меньшее время отклика программы, но повышают потребление ресурсов - это особенно заметно на старых машинах и системах, ядро которых не поддерживает приоритета реального времени. Если наблюдается прерывистый звук, попробуйте увеличить размер буфера.</translation>
    </message>
    <message>
        <source>Here you can select your preferred audio-interface. Depending on the configuration of your system during compilation time you can choose between ALSA, JACK, OSS and more. Below you see a box which offers controls to setup the selected audio-interface.</source>
        <translation>Пожалуйста, выберите звуковую систему. В зависимости от конфигурации во время компилирования программы, вы можете использовать ALSA, JACK, OSS и другие. В нижней части окна настройки можно задать специфические параметры выбранной системы.</translation>
    </message>
    <message>
        <source>Here you can select your preferred MIDI-interface. Depending on the configuration of your system during compilation time you can choose between ALSA, OSS and more. Below you see a box which offers controls to setup the selected MIDI-interface.</source>
        <translation>Пожалуйста, выберите интерфейс MIDI. В зависимости от конфигурации во время компилирования программы, вы можете использовать ALSA, OSS и другие. В нижней части окна настройки можно задать специфические параметры выбранного интерфейса.</translation>
    </message>
    <message>
        <source>LMMS working directory</source>
        <translation>Рабочий каталог LMMS</translation>
    </message>
    <message>
        <source>VST-plugin directory</source>
        <translation>Каталог модулей VST</translation>
    </message>
    <message>
        <source>Choose LMMS working directory</source>
        <translation>Выбор рабочего каталога LMMS</translation>
    </message>
    <message>
        <source>Choose your VST-plugin directory</source>
        <translation>Выбор каталога для модулей VST</translation>
    </message>
    <message>
        <source>Performance settings</source>
        <translation>Параметры производительности</translation>
    </message>
    <message>
        <source>UI effects vs. performance</source>
        <translation>Визуальные эффекты/производительность</translation>
    </message>
    <message>
        <source>Frames: %1
Latency: %2 ms</source>
        <translation>Фрагментов: %1
Отклик: %2</translation>
    </message>
    <message>
        <source>Artwork directory</source>
        <translation>Каталог с элементами оформления</translation>
    </message>
    <message>
        <source>Choose artwork-theme directory</source>
        <translation>Выбор каталога с темой оформления для LMMS</translation>
    </message>
    <message>
        <source>Display volume as dBV </source>
        <translation>Отображать громкости в децибелах (напр.) </translation>
    </message>
    <message>
        <source>FL Studio installation directory</source>
        <translation>Каталог установки FL Studio</translation>
    </message>
    <message>
        <source>STK rawwave directory</source>
        <translation>Каталог STK</translation>
    </message>
    <message>
        <source>Choose FL Studio installation directory</source>
        <translation>Выбор каталога FL Studio</translation>
    </message>
    <message>
        <source>Choose LADSPA plugin directory</source>
        <translation>Выбор каталога с модулями LADSPA </translation>
    </message>
    <message>
        <source>Choose STK rawwave directory</source>
        <translation>Выбор каталога STK</translation>
    </message>
    <message>
        <source>Enable tooltips</source>
        <translation>Включить всплывающие подсказки</translation>
    </message>
    <message>
        <source>Show restart warning after changing settings</source>
        <translation>Показывать предупреждение после изменения настроек</translation>
    </message>
    <message>
        <source>Compress project files per default</source>
        <translation>По умолчанию сжимать файлы проектов</translation>
    </message>
    <message>
        <source>HQ-mode for output audio-device</source>
        <translation>Режим высокого качества для вывода звука</translation>
    </message>
    <message>
        <source>Paths</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>LADSPA plugin paths</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Default Soundfont File</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Background artwork</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Choose default SoundFont</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Choose background artwork</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>One instrument track window mode</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Compact track buttons</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Sync VST plugins to host playback</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Enable note labels in piano roll</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Enable waveform display by default</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Smooth scroll in Song Editor</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Enable auto save feature</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Show playback cursor in AudioFileProcessor</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>sf2Instrument</name>
    <message>
        <source>Bank</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Patch</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Gain</source>
        <translation>Усиление</translation>
    </message>
    <message>
        <source>Reverb</source>
        <translation>Эхо</translation>
    </message>
    <message>
        <source>Reverb Roomsize</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Reverb Damping</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Reverb Width</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Reverb Level</source>
        <translation>Уровень эха</translation>
    </message>
    <message>
        <source>Chorus</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Chorus Lines</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Chorus Level</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Chorus Speed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Chorus Depth</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>sf2InstrumentView</name>
    <message>
        <source>Open other SoundFont file</source>
        <translation>Открыть другой файл SoundFront</translation>
    </message>
    <message>
        <source>Click here to open another SF2 file</source>
        <translation>Нажмите здесь чтобы открыть другой файл SF2</translation>
    </message>
    <message>
        <source>Choose the patch</source>
        <translation>Выбрать патч</translation>
    </message>
    <message>
        <source>Gain</source>
        <translation>Усиление</translation>
    </message>
    <message>
        <source>Apply reverb (if supported)</source>
        <translation>Создать эхо (если поддерживается)</translation>
    </message>
    <message>
        <source>This button enables the reverb effect. This is useful for cool effects, but only works on files that support it.</source>
        <translation>Эта кнопка включает эффект эха. Это может пригодиться, но работает не для всех файлов.</translation>
    </message>
    <message>
        <source>Reverb Roomsize:</source>
        <translation>Размер помещения:</translation>
    </message>
    <message>
        <source>Reverb Damping:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Reverb Width:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Reverb Level:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Apply chorus (if supported)</source>
        <translation>Создать эффект хора (если поддерживается)</translation>
    </message>
    <message>
        <source>This button enables the chorus effect. This is useful for cool echo effects, but only works on files that support it.</source>
        <translation>Эта кнопка включает эффект хора. Это может пригодиться, но работает не для всех файлов.</translation>
    </message>
    <message>
        <source>Chorus Lines:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Chorus Level:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Chorus Speed:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Chorus Depth:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Open SoundFont file</source>
        <translation>Открыть файл SoundFront</translation>
    </message>
    <message>
        <source>SoundFont2 Files (*.sf2)</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>sfxrInstrument</name>
    <message>
        <source>Wave Form</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>sidInstrument</name>
    <message>
        <source>Cutoff</source>
        <translation>Срез</translation>
    </message>
    <message>
        <source>Resonance</source>
        <translation>Усиление</translation>
    </message>
    <message>
        <source>Filter type</source>
        <translation>Тип фильтра</translation>
    </message>
    <message>
        <source>Voice 3 off</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Volume</source>
        <translation>Громкость</translation>
    </message>
    <message>
        <source>Chip model</source>
        <translation>Модель чипа</translation>
    </message>
</context>
<context>
    <name>sidInstrumentView</name>
    <message>
        <source>Volume:</source>
        <translation>Громкость:</translation>
    </message>
    <message>
        <source>Resonance:</source>
        <translation>Усиление:</translation>
    </message>
    <message>
        <source>Cutoff frequency:</source>
        <translation>Частота среза:</translation>
    </message>
    <message>
        <source>High-Pass filter </source>
        <translation>ФВЧ</translation>
    </message>
    <message>
        <source>Band-Pass filter </source>
        <translation>ППФ</translation>
    </message>
    <message>
        <source>Low-Pass filter </source>
        <translation>ФНЧ</translation>
    </message>
    <message>
        <source>Voice3 Off </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>MOS6581 SID </source>
        <translation></translation>
    </message>
    <message>
        <source>MOS8580 SID </source>
        <translation></translation>
    </message>
    <message>
        <source>Attack:</source>
        <translation>Длительность нарастания:</translation>
    </message>
    <message>
        <source>Attack rate determines how rapidly the output of Voice %1 rises from zero to peak amplitude.</source>
        <translation>Длительность нарастания определяет, насколько быстро громкость %1-го голоса возрастает от нуля до наибольшего значения.</translation>
    </message>
    <message>
        <source>Decay:</source>
        <translation>Длительность спада:</translation>
    </message>
    <message>
        <source>Decay rate determines how rapidly the output falls from the peak amplitude to the selected Sustain level.</source>
        <translation>Длительность спада определяет, насколько быстро громкость падает от максимума до остаточного уровня.</translation>
    </message>
    <message>
        <source>Sustain:</source>
        <translation>Остаточный уровень:</translation>
    </message>
    <message>
        <source>Output of Voice %1 will remain at the selected Sustain amplitude as long as the note is held.</source>
        <translation>Громкость %1-го голоса будет оставаться на этом уровне, пока длится нота.</translation>
    </message>
    <message>
        <source>Release:</source>
        <translation>Длительность исчезновения:</translation>
    </message>
    <message>
        <source>The output of of Voice %1 will fall from Sustain amplitude to zero amplitude at the selected Release rate.</source>
        <translation>Громкость %1-го голоса будет падать от остаточного уровня до нуля с указанной здесь скоростью.</translation>
    </message>
    <message>
        <source>Pulse Width:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>The Pulse Width resolution allows the width to be smoothly swept with no discernable stepping. The Pulse waveform on Oscillator %1 must be selected to have any audible effect.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Coarse:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>The Coarse detuning allows to detune Voice %1 one octave up or down.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Pulse Wave</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Triangle Wave</source>
        <translation>Треугольник</translation>
    </message>
    <message>
        <source>SawTooth</source>
        <translation>Пила </translation>
    </message>
    <message>
        <source>Noise</source>
        <translation>Шум</translation>
    </message>
    <message>
        <source>Sync</source>
        <translation>Синхронизировать</translation>
    </message>
    <message>
        <source>Sync synchronizes the fundamental frequency of Oscillator %1 with the fundamental frequency of Oscillator %2 producing &quot;Hard Sync&quot; effects.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Ring-Mod</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Ring-mod replaces the Triangle Waveform output of Oscillator %1 with a &quot;Ring Modulated&quot; combination of Oscillators %1 and %2.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Filtered</source>
        <translation>Фильтровать</translation>
    </message>
    <message>
        <source>When Filtered is on, Voice %1 will be processed through the Filter. When Filtered is off, Voice %1 appears directly at the output, and the Filter has no effect on it.</source>
        <translation>Если этот флажок установлен, то %1-й голос будет проходить через фильтр. Иначе голос №%1 будет подаваться прямо на выход.</translation>
    </message>
    <message>
        <source>Test</source>
        <translation>Флажок</translation>
    </message>
    <message>
        <source>Test, when set, resets and locks Oscillator %1 at zero until Test is turned off.</source>
        <translation>Если «флажок» установлен, то %1-й генератор выдаёт нулевой сигнал (пока флажок не снимется).</translation>
    </message>
</context>
<context>
    <name>song</name>
    <message>
        <source>Tempo</source>
        <translation>Темп</translation>
    </message>
    <message>
        <source>Master volume</source>
        <translation>Общая громкость</translation>
    </message>
    <message>
        <source>Master pitch</source>
        <translation>Общая высота тона</translation>
    </message>
    <message>
        <source>Project saved</source>
        <translation>Проект сохранён</translation>
    </message>
    <message>
        <source>The project %1 is now saved.</source>
        <translation>Проект %1 сохранён.</translation>
    </message>
    <message>
        <source>Project NOT saved.</source>
        <translation>Проект НЕ СОХРАНЁН.</translation>
    </message>
    <message>
        <source>The project %1 was not saved!</source>
        <translation>Проект %1 не сохранён!</translation>
    </message>
    <message>
        <source>Import file</source>
        <translation>Импорт файла</translation>
    </message>
    <message>
        <source>Empty project</source>
        <translation>Проект пуст</translation>
    </message>
    <message>
        <source>This project is empty so exporting makes no sense. Please put some items into Song Editor first!</source>
        <translation>Проект ничего не содержит, так что и экспортировать нечего. Сначала добавьте хотя бы одну дорожку с помощью секвенсора!</translation>
    </message>
    <message>
        <source>untitled</source>
        <translation>Неназванное</translation>
    </message>
    <message>
        <source>Select file for project-export...</source>
        <translation>Выбор файла для экспорта проекта...</translation>
    </message>
    <message>
        <source>MIDI sequences</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>FL Studio projects</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>All file types</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Hydrogen projects</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Select directory for writing exported tracks...</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>songEditor</name>
    <message>
        <source>Click here, if you want to stop playing of your song. The song-position-marker will be set to the start of your song.</source>
        <translation>Нажмите сюда, если вы хотите остановить воспроизведение мелодии. Курсор при этом будет установлен на начало композиции.</translation>
    </message>
    <message>
        <source>Could not open file</source>
        <translation>Не могу открыть файл</translation>
    </message>
    <message>
        <source>Could not write file</source>
        <translation>Не могу записать файл</translation>
    </message>
    <message>
        <source>Song-Editor</source>
        <translation>Финальный секвенсор</translation>
    </message>
    <message>
        <source>Click here, if you want to play your whole song. Playing will be started at the song-position-marker (green). You can also move it while playing.</source>
        <translation>Нажмите, чтобы прослушать созданную мелодию. Воспроизведение начнётся с позиции курсора (зелёный треугольник); вы можете двигать его во время проигрывания.</translation>
    </message>
    <message>
        <source>Play song (Space)</source>
        <translation>Начать воспроизведение (Пробел)</translation>
    </message>
    <message>
        <source>Stop song (Space)</source>
        <translation>Остановить воспроизведение (Пробел)</translation>
    </message>
    <message>
        <source>Add beat/bassline</source>
        <translation>Добавить ритм/басы</translation>
    </message>
    <message>
        <source>Add sample-track</source>
        <translation>Добавить файл</translation>
    </message>
    <message>
        <source>Draw mode</source>
        <translation>Режим рисования</translation>
    </message>
    <message>
        <source>Edit mode (select and move)</source>
        <translation>Правка (выделение/перемещение)</translation>
    </message>
    <message>
        <source>Record samples from Audio-device</source>
        <translation>Записать образец со звукового устройства</translation>
    </message>
    <message>
        <source>Record samples from Audio-device while playing song or BB track</source>
        <translation>Записать ноты со звуковой платы во время воспроизведения композиции или лейтмотива</translation>
    </message>
    <message>
        <source>Add automation-track</source>
        <translation>Добавить дорожку автоматизации</translation>
    </message>
    <message>
        <source>Could not open file %1. You probably have no permissions to read this file.
 Please make sure to have at least read permissions to the file and try again.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Error in file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>The file %1 seems to contain errors and therefore can&apos;t be loaded.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Tempo</source>
        <translation type="unfinished">Темп</translation>
    </message>
    <message>
        <source>TEMPO/BPM</source>
        <translation type="unfinished">ТЕМП/BPM</translation>
    </message>
    <message>
        <source>tempo of song</source>
        <translation type="unfinished">Темп мелодии</translation>
    </message>
    <message>
        <source>The tempo of a song is specified in beats per minute (BPM). If you want to change the tempo of your song, change this value. Every measure has four beats, so the tempo in BPM specifies, how many measures / 4 should be played within a minute (or how many measures should be played within four minutes).</source>
        <translation type="unfinished">Это значение задаёт темп мелодии в ударах в минуту (англ. аббр. BPM). На каждый такт приходится четыре удара, так что темп в ударах в минуту фактически указывает, сколько четвертей такта проигрывается за минуту (или, что то же, количество тактов, проигрываемых за четыре минуты).</translation>
    </message>
    <message>
        <source>High quality mode</source>
        <translation type="unfinished">Высокое качество</translation>
    </message>
    <message>
        <source>Master volume</source>
        <translation type="unfinished">Общая громкость</translation>
    </message>
    <message>
        <source>master volume</source>
        <translation type="unfinished">Общая громкость</translation>
    </message>
    <message>
        <source>Master pitch</source>
        <translation type="unfinished">Общая высота тона</translation>
    </message>
    <message>
        <source>master pitch</source>
        <translation type="unfinished">Общая высота тона</translation>
    </message>
    <message>
        <source>Value: %1%</source>
        <translation type="unfinished">Значение: %1%</translation>
    </message>
    <message>
        <source>Value: %1 semitones</source>
        <translation type="unfinished">Значение: %1 полутон(а/ов)</translation>
    </message>
    <message>
        <source>Could not open %1 for writing. You probably are not permitted to write to this file. Please make sure you have write-access to the file and try again.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>spectrumAnalyzerControlDialog</name>
    <message>
        <source>Linear spectrum</source>
        <translation>Линейный спектр</translation>
    </message>
    <message>
        <source>Linear Y axis</source>
        <translation>Линейная ось ординат</translation>
    </message>
</context>
<context>
    <name>spectrumAnalyzerControls</name>
    <message>
        <source>Linear spectrum</source>
        <translation>Линейный спектр</translation>
    </message>
    <message>
        <source>Linear Y-axis</source>
        <translation>Линейная ось ординат</translation>
    </message>
    <message>
        <source>Channel mode</source>
        <translation>Режим канала</translation>
    </message>
</context>
<context>
    <name>stereoEnhancerControlDialog</name>
    <message>
        <source>WIDE</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Width:</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>stereoEnhancerControls</name>
    <message>
        <source>Width</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>stereoMatrixControlDialog</name>
    <message>
        <source>Left to Left Vol:</source>
        <translation>От левого на левый:</translation>
    </message>
    <message>
        <source>Left to Right Vol:</source>
        <translation>От левого на правый:</translation>
    </message>
    <message>
        <source>Right to Left Vol:</source>
        <translation>От правого на левый:</translation>
    </message>
    <message>
        <source>Right to Right Vol:</source>
        <translation>От правого на правый:</translation>
    </message>
</context>
<context>
    <name>stereoMatrixControls</name>
    <message>
        <source>Left to Left</source>
        <translation>От левого на левый</translation>
    </message>
    <message>
        <source>Left to Right</source>
        <translation>От левого на правый</translation>
    </message>
    <message>
        <source>Right to Left</source>
        <translation>От правого на левый</translation>
    </message>
    <message>
        <source>Right to Right</source>
        <translation>От правого на правый</translation>
    </message>
</context>
<context>
    <name>timeLine</name>
    <message>
        <source>Enable/disable auto-scrolling</source>
        <translation>Вкл/выкл автопрокрутку</translation>
    </message>
    <message>
        <source>Enable/disable loop-points</source>
        <translation>Вкл/выкл точки перемотки</translation>
    </message>
    <message>
        <source>After stopping go back to begin</source>
        <translation>После останова переходить к началу</translation>
    </message>
    <message>
        <source>After stopping go back to position at which playing was started</source>
        <translation>После останова переходить к месту, с которого началось воспроизведение</translation>
    </message>
    <message>
        <source>After stopping keep position</source>
        <translation>Оставаться на месте останова</translation>
    </message>
    <message>
        <source>Hint</source>
        <translation>Подсказка</translation>
    </message>
    <message>
        <source>Press &lt;Ctrl&gt; to disable magnetic loop points.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Hold &lt;Shift&gt; to move the begin loop point; Press &lt;Ctrl&gt; to disable magnetic loop points.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>track</name>
    <message>
        <source>Muted</source>
        <translation>Заглушён</translation>
    </message>
    <message>
        <source>Solo</source>
        <translation>Моно</translation>
    </message>
</context>
<context>
    <name>trackContentObject</name>
    <message>
        <source>Muted</source>
        <translation>Заглушён</translation>
    </message>
</context>
<context>
    <name>trackContentObjectView</name>
    <message>
        <source>Current position</source>
        <translation>Позиция</translation>
    </message>
    <message>
        <source>Hint</source>
        <translation>Подсказка</translation>
    </message>
    <message>
        <source>Press &lt;Ctrl&gt; and drag to make a copy.</source>
        <translation>Нажмите &lt;Ctrl&gt; и отпустите, чтобы создать копию.</translation>
    </message>
    <message>
        <source>Current length</source>
        <translation>Длительность</translation>
    </message>
    <message>
        <source>Press &lt;Ctrl&gt; for free resizing.</source>
        <translation>Для свободного перемещения нажмите &lt;Ctrl&gt;.</translation>
    </message>
    <message>
        <source>%1:%2 (%3:%4 to %5:%6)</source>
        <translation>%1:%2 (от %3:%4 до %5:%6)</translation>
    </message>
    <message>
        <source>Delete (middle mousebutton)</source>
        <translation>Удалить (средняя кнопка мыши)</translation>
    </message>
    <message>
        <source>Cut</source>
        <translation>Вырезать</translation>
    </message>
    <message>
        <source>Copy</source>
        <translation>Копировать</translation>
    </message>
    <message>
        <source>Paste</source>
        <translation>Вставить</translation>
    </message>
    <message>
        <source>Mute/unmute (&lt;Ctrl&gt; + middle click)</source>
        <translation>Заглушить/включить (Crl + средняя кнопка мыши)</translation>
    </message>
</context>
<context>
    <name>trackOperationsWidget</name>
    <message>
        <source>Clone this track</source>
        <translation>Копировать дорожку</translation>
    </message>
    <message>
        <source>Remove this track</source>
        <translation>Удалить дорожку</translation>
    </message>
    <message>
        <source>Press &lt;Ctrl&gt; while clicking on move-grip to begin a new drag&apos;n&apos;drop-action.</source>
        <translation>Для перетаскивания в другое окно нажмите &lt;Ctrl&gt; при нажатии на эту полоску.</translation>
    </message>
    <message>
        <source>Actions for this track</source>
        <translation>Действия для этой дорожки</translation>
    </message>
    <message>
        <source>Mute</source>
        <translation>Заглушить</translation>
    </message>
    <message>
        <source>Mute this track</source>
        <translation>Отключить дорожку</translation>
    </message>
    <message>
        <source>Solo</source>
        <translation>Моно</translation>
    </message>
</context>
<context>
    <name>vestigeInstrument</name>
    <message>
        <source>Failed loading VST-plugin</source>
        <translation>Не смог загрузить модуль VST</translation>
    </message>
    <message>
        <source>The VST-plugin %1 could not be loaded for some reason.
If it runs with other VST-software under Linux, please contact an LMMS-developer!</source>
        <translation>Отчего-то модуль VST %1 не мог быть загружен.
Если другое программное обеспечение VST работает у Вас под Linux&apos;ом, свяжитесь с разработчиком LMMS!</translation>
    </message>
    <message>
        <source>Loading plugin</source>
        <translation>Загрузка модуля</translation>
    </message>
    <message>
        <source>Please wait while loading VST-plugin...</source>
        <translation>Подождите, пока загружается модуль VST...</translation>
    </message>
</context>
<context>
    <name>vibed</name>
    <message>
        <source>String %1 volume</source>
        <translation>Громкость %1-й струны</translation>
    </message>
    <message>
        <source>String %1 stiffness</source>
        <translation>Жёсткость %1-й струны</translation>
    </message>
    <message>
        <source>Pick %1 position</source>
        <translation>Лад %1</translation>
    </message>
    <message>
        <source>Pickup %1 position</source>
        <translation>Положение %1-го звукоснимателя</translation>
    </message>
    <message>
        <source>Pan %1</source>
        <translation>Стереобаланс %1</translation>
    </message>
    <message>
        <source>Detune %1</source>
        <translation>Расстройка %1</translation>
    </message>
    <message>
        <source>Fuzziness %1 </source>
        <translation>Нечёткость %1</translation>
    </message>
    <message>
        <source>Length %1</source>
        <translation>Длина %1</translation>
    </message>
    <message>
        <source>Impulse %1</source>
        <translation>Импульс %1</translation>
    </message>
    <message>
        <source>Octave %1</source>
        <translation>Октава %1</translation>
    </message>
</context>
<context>
    <name>vibedView</name>
    <message>
        <source>Volume:</source>
        <translation>Громкость:</translation>
    </message>
    <message>
        <source>The &apos;V&apos; knob sets the volume of the selected string.</source>
        <translation>Регулятор &apos;V&apos; устанавливает громкость текущей струны.</translation>
    </message>
    <message>
        <source>String stiffness:</source>
        <translation>Жёсткость:</translation>
    </message>
    <message>
        <source>The &apos;S&apos; knob sets the stiffness of the selected string.  The stiffness of the string affects how long the string will ring out.  The lower the setting, the longer the string will ring.</source>
        <translation>Регулятор &apos;S&apos; устанавливает жёсткость текущей струны. Этот параметр отвечает за длительность звучания струны (чем больше значение жёсткости, тем тольше звенит струна).</translation>
    </message>
    <message>
        <source>Pick position:</source>
        <translation>Лад:</translation>
    </message>
    <message>
        <source>The &apos;P&apos; knob sets the position where the selected string will be &apos;picked&apos;.  The lower the setting the closer the pick is to the bridge.</source>
        <translation>Регулятор &apos;P&apos; устанавливает место струны, где она будет „прижата“. Чем ниже значение, тем ближе это место будет к кобылке.</translation>
    </message>
    <message>
        <source>Pickup position:</source>
        <translation>Положение звукоснимателя:</translation>
    </message>
    <message>
        <source>The &apos;PU&apos; knob sets the position where the vibrations will be monitored for the selected string.  The lower the setting, the closer the pickup is to the bridge.</source>
        <translation>Регулятор &apos;PU&apos; устанавливает место струны, откуда будет сниматься звук. Чем ниже значение, тем ближе это место будет к кобылке.</translation>
    </message>
    <message>
        <source>Pan:</source>
        <translation>Стереобаланс:</translation>
    </message>
    <message>
        <source>The Pan knob determines the location of the selected string in the stereo field.</source>
        <translation>Этот регулятор устанавливает стереобаланс для текущей струны.</translation>
    </message>
    <message>
        <source>Detune:</source>
        <translation>Расстройка:</translation>
    </message>
    <message>
        <source>The Detune knob modifies the pitch of the selected string.  Settings less than zero will cause the string to sound flat.  Settings greater than zero will cause the string to sound sharp.</source>
        <translation>Регулятор  расстройки изменяет сдвиг частоты для текущей струны. Отрицательные значения заставят струну звучать бемольно, положительные — диезно. </translation>
    </message>
    <message>
        <source>Fuzziness:</source>
        <translation>Нечёткость:</translation>
    </message>
    <message>
        <source>The Slap knob adds a bit of fuzz to the selected string which is most apparent during the attack, though it can also be used to make the string sound more &apos;metallic&apos;.</source>
        <translation>Этот регулятор добавляет размытости звуку, что наиболее заметно во время нарастания, впрочем это может использоваться чтобы сделать звук более „металлическим“.</translation>
    </message>
    <message>
        <source>Length:</source>
        <translation>Длина:</translation>
    </message>
    <message>
        <source>The Length knob sets the length of the selected string.  Longer strings will both ring longer and sound brighter, however, they will also eat up more CPU cycles.</source>
        <translation>Регулятор длины устанавливает длину текущей струны. Чем длиннее струна, тем более чистый и долгий звук она даёт; однако это требует больше ресурсов ЦП.</translation>
    </message>
    <message>
        <source>Impulse or initial state</source>
        <translation>Начальная скорость/начальное состояние</translation>
    </message>
    <message>
        <source>The &apos;Imp&apos; selector determines whether the waveform in the graph is to be treated as an impulse imparted to the string by the pick or the initial state of the string.</source>
        <translation>Переключатель „Imp“ устанавливает режим работы струны: если он включён, то указанная форма сигнала интерпретируется как начальный импульс, иначе — как начальная форма струны.</translation>
    </message>
    <message>
        <source>Octave</source>
        <translation>Октава</translation>
    </message>
    <message>
        <source>The Octave selector is used to choose which harmonic of the note the string will ring at.  For example, &apos;-2&apos; means the string will ring two octaves below the fundamental, &apos;F&apos; means the string will ring at the fundamental, and &apos;6&apos; means the string will ring six octaves above the fundamental.</source>
        <translation>Переключатель октав позволяет указать гармонику основной частоты, на которой будет звучать струна. Например, „-2“ означает, что струна будет звучать двумя октавами ниже основной частоты, „F“ заставит струну звенеть на основной частоте инструмента, а „6“ — на частоте, на шесть октав более высокой, чем основная.</translation>
    </message>
    <message>
        <source>Impulse Editor</source>
        <translation>Редактор формы импульса</translation>
    </message>
    <message>
        <source>The waveform editor provides control over the initial state or impulse that is used to start the string vibrating.  The buttons to the right of the graph will initialize the waveform to the selected type.  The &apos;?&apos; button will load a waveform from a file--only the first 128 samples will be loaded.

The waveform can also be drawn in the graph.

The &apos;S&apos; button will smooth the waveform.

The &apos;N&apos; button will normalize the waveform.</source>
        <translation>Редактор формы позволяет явно указать профиль струны в начальный момент времени, либо её начальный импульс (в заисимости от состояния переключателя „Imp“). Кнопки справа от рисунка позволяют задавать некоторые стандартные формы, причём кнопка &apos;?&apos; служит для задания формы из произвольного звукового файла (загружаются первые 128 элементов выборки).

Также форма сигнала может быть просто нарисована с помощью мыши.

Кнопка &apos;S&apos; сгладит текущую форму.

Кнопка &apos;N&apos; нормализует уровень.</translation>
    </message>
    <message>
        <source>Vibed models up to nine independently vibrating strings.  The &apos;String&apos; selector allows you to choose which string is being edited.  The &apos;Imp&apos; selector chooses whether the graph represents an impulse or the initial state of the string.  The &apos;Octave&apos; selector chooses which harmonic the string should vibrate at.

The graph allows you to control the initial state or impulse used to set the string in motion.

The &apos;V&apos; knob controls the volume.  The &apos;S&apos; knob controls the string&apos;s stiffness.  The &apos;P&apos; knob controls the pick position.  The &apos;PU&apos; knob controls the pickup position.

&apos;Pan&apos; and &apos;Detune&apos; hopefully don&apos;t need explanation.  The &apos;Slap&apos; knob adds a bit of fuzz to the sound of the string.

The &apos;Length&apos; knob controls the length of the string.

The LED in the lower right corner of the waveform editor determines whether the string is active in the current instrument.</source>
        <translation>Инструмент „Vibed“ моделирует до девяти независимых одновременно звучащих струн. 

Переключатель „Strings“  позволяет выбрать струну, чьи свойства редактируются.

Переключатель „Imp“ устанавливает режим работы струны: если он включён, то указанная форма сигнала интерпретируется как начальный импульс, иначе — как начальная форма струны.

Переключатель „Octave“  позволяет указать гармонику основной частоты, на которой будет звучать струна.

Редактор формы позволяет явно указать профиль струны в начальный момент времени, либо её начальный импульс.

Регулятор &apos;V&apos; устанавливает громкость текущей струны, &apos;S&apos; — жёсткость, &apos;P&apos; — место, где прижата струна, а  &apos;PU&apos;&apos; — положение звукоснимателя

Регуляторы расстройки и стереобаланса, есть надежда, не нуждаются в объяснениях.

Ручка „Длина“ регулирует длину струны

Индикатор-переключатель слева снизу определяет, включена ли текущая струна.</translation>
    </message>
    <message>
        <source>Enable waveform</source>
        <translation>Включить</translation>
    </message>
    <message>
        <source>Click here to enable/disable waveform.</source>
        <translation>Нажмите, чтобы включить/выключить сигнал.</translation>
    </message>
    <message>
        <source>String</source>
        <translation>Струна</translation>
    </message>
    <message>
        <source>The String selector is used to choose which string the controls are editing.  A Vibed instrument can contain up to nine independently vibrating strings.  The LED in the lower right corner of the waveform editor indicates whether the selected string is active.</source>
        <translation>Переключатель струн позволяет выбрать струну, чьи свойства редактируются. Инструмент Vibed содержит до девяти независимо звучащих струн, индикатор в левом нижнем углу показывает, активна ли текущая струна (т. е. будет ли она слышна).</translation>
    </message>
    <message>
        <source>Sine wave</source>
        <translation>Синусоида</translation>
    </message>
    <message>
        <source>Use a sine-wave for current oscillator.</source>
        <translation>Генерировать гармонический (синусоидальный) сигнал.</translation>
    </message>
    <message>
        <source>Triangle wave</source>
        <translation>Треугольник</translation>
    </message>
    <message>
        <source>Use a triangle-wave for current oscillator.</source>
        <translation>Генерировать треугольный сигнал.</translation>
    </message>
    <message>
        <source>Saw wave</source>
        <translation>Пила</translation>
    </message>
    <message>
        <source>Use a saw-wave for current oscillator.</source>
        <translation>Генерировать пилообразный сигнал.</translation>
    </message>
    <message>
        <source>Square wave</source>
        <translation>Меандр</translation>
    </message>
    <message>
        <source>Use a square-wave for current oscillator.</source>
        <translation>Генерировать меандр.</translation>
    </message>
    <message>
        <source>White noise wave</source>
        <translation>Белый шум</translation>
    </message>
    <message>
        <source>Use white-noise for current oscillator.</source>
        <translation>Генерировать белый шум.</translation>
    </message>
    <message>
        <source>User defined wave</source>
        <translation>Пользовательская</translation>
    </message>
    <message>
        <source>Use a user-defined waveform for current oscillator.</source>
        <translation>Задать форму сигнала.</translation>
    </message>
    <message>
        <source>Smooth</source>
        <translation>Сгладить</translation>
    </message>
    <message>
        <source>Click here to smooth waveform.</source>
        <translation>Щёлкните чтобы сгладить форму сигнала.</translation>
    </message>
    <message>
        <source>Normalize</source>
        <translation>Нормализовать</translation>
    </message>
    <message>
        <source>Click here to normalize waveform.</source>
        <translation>Нажмите, чтобы нормализовать сигнал.</translation>
    </message>
    <message>
        <source>&amp;Help</source>
        <translation>&amp;Справка</translation>
    </message>
</context>
<context>
    <name>visualizationWidget</name>
    <message>
        <source>click to enable/disable visualization of master-output</source>
        <translation>Нажмите, чтобы включить/выключить визуализацию</translation>
    </message>
    <message>
        <source>Click to enable</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>voiceObject</name>
    <message>
        <source>Voice %1 pulse width</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Voice %1 attack</source>
        <translation>Нарастание %1-го голоса</translation>
    </message>
    <message>
        <source>Voice %1 decay</source>
        <translation>Спад %1-го голоса</translation>
    </message>
    <message>
        <source>Voice %1 sustain</source>
        <translation>Остаточный уровень для %1-го голоса</translation>
    </message>
    <message>
        <source>Voice %1 release</source>
        <translation>Исчезновения %1-го голоса</translation>
    </message>
    <message>
        <source>Voice %1 coarse detuning</source>
        <translation>Расстройка %1-го голоса (грубо)</translation>
    </message>
    <message>
        <source>Voice %1 wave shape</source>
        <translation>Форма сигнала для %1-го голоса</translation>
    </message>
    <message>
        <source>Voice %1 sync</source>
        <translation>Синхронизация %1-го голоса</translation>
    </message>
    <message>
        <source>Voice %1 ring modulate</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Voice %1 filtered</source>
        <translation>Фильтровать %1-й голос</translation>
    </message>
    <message>
        <source>Voice %1 test</source>
        <translation type="unfinished"></translation>
    </message>
</context>
</TS>
