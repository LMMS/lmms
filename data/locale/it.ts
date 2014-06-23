<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.0" language="it_IT">
<context>
    <name>AboutDialog</name>
    <message>
        <source>About LMMS</source>
        <translation>About LMMS</translation>
    </message>
    <message>
        <source>LMMS (Linux MultiMedia Studio)</source>
        <translation>LMMS (Linux MultiMedia Studio)</translation>
    </message>
    <message>
        <source>Version %1 (%2/%3, Qt %4, %5)</source>
        <translation>Versione %1 (%2/%3, Qt %4, %5)</translation>
    </message>
    <message>
        <source>About</source>
        <translation>Informazioni su</translation>
    </message>
    <message>
        <source>LMMS - easy music production for everyone</source>
        <translation>LMMS - Produzione musicale semplice per tutti</translation>
    </message>
    <message>
        <source>Authors</source>
        <translation>Autori</translation>
    </message>
    <message>
        <source>Translation</source>
        <translation>Traduzione</translation>
    </message>
    <message>
        <source>Current language not translated (or native English).

If you&apos;re interested in translating LMMS in another language or want to improve existing translations, you&apos;re welcome to help us! Simply contact the maintainer!</source>
        <translatorcomment>Se hai partecipato alla traduzione ed il tuo nome non è presente in questa lista, aggiungilo!</translatorcomment>
        <translation>Roberto Giaconia &lt;derobyj@gmail.com&gt;</translation>
    </message>
    <message>
        <source>License</source>
        <translation>Licenza</translation>
    </message>
    <message>
        <source>Copyright (c) 2004-2014, LMMS developers</source>
        <translation>Copyright (c) 2004-2014, LMMS developers</translation>
    </message>
    <message>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;a href=&quot;http://lmms.sourceforge.net&quot;&gt;&lt;span style=&quot; text-decoration: underline; color:#0000ff;&quot;&gt;http://lmms.sourceforge.net&lt;/span&gt;&lt;/a&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;a href=&quot;http://lmms.sourceforge.net&quot;&gt;&lt;span style=&quot; text-decoration: underline; color:#0000ff;&quot;&gt;http://lmms.sourceforge.net&lt;/span&gt;&lt;/a&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
</context>
<context>
    <name>AudioAlsa::setupWidget</name>
    <message>
        <source>DEVICE</source>
        <translation>PERIFERICA</translation>
    </message>
    <message>
        <source>CHANNELS</source>
        <translation>CANALI</translation>
    </message>
</context>
<context>
    <name>AudioFileProcessorView</name>
    <message>
        <source>Open other sample</source>
        <translation>Apri un altro campione</translation>
    </message>
    <message>
        <source>Click here, if you want to open another audio-file. A dialog will appear where you can select your file. Settings like looping-mode, start and end-points, amplify-value, and so on are not reset. So, it may not sound like the original sample.</source>
        <translation>Clicca qui per aprire un altro file audio. Apparirà una finestra di dialogo dove sarà possibile scegliere il file. Impostazioni come la modalità ripetizione, amplificazione e così via non vengono reimpostate, pertanto potrebbe non suonare come il file originale.</translation>
    </message>
    <message>
        <source>Reverse sample</source>
        <translation>Inverti il campione</translation>
    </message>
    <message>
        <source>If you enable this button, the whole sample is reversed. This is useful for cool effects, e.g. a reversed crash.</source>
        <translation>Attivando questo pulsante, l&apos;intero campione viene invertito. Ciò è utile per effetti particolari, ad es. un crash invertito.</translation>
    </message>
    <message>
        <source>Loop sample at start- and end-point</source>
        <translation>Ripeti il campione tra i punti di inizio e fine</translation>
    </message>
    <message>
        <source>Here you can set, whether looping-mode is enabled. If enabled, AudioFileProcessor loops between start and end-points of a sample until the whole note is played. This is useful for things like string and choir samples.</source>
        <translation>Qui è possibile impostare se la modalità ripetizione è attiva. AudioFileProcessor riproduce tra i punti di inizio e fine di un campione finché tutta la nota è stata suonata. Ciò è utile per campioni di strumenti a corda e cori.</translation>
    </message>
    <message>
        <source>Amplify:</source>
        <translation>Amplificazione:</translation>
    </message>
    <message>
        <source>With this knob you can set the amplify ratio. When you set a value of 100% your sample isn&apos;t changed. Otherwise it will be amplified up or down (your actual sample-file isn&apos;t touched!)</source>
        <translation>Questa manopola regola l&apos;amplificaione. Con un valore pari a 100% il campione non viene modificato, altrimenti verrà amplificato della percentuale specificata (il file originale non viene modificato!)</translation>
    </message>
    <message>
        <source>Startpoint:</source>
        <translation>Punto di inizio:</translation>
    </message>
    <message>
        <source>With this knob you can set the point where AudioFileProcessor should begin playing your sample. If you enable looping-mode, this is the point to which AudioFileProcessor returns if a note is longer than the sample between the start and end-points.</source>
        <translation>Questa manopola regola il punto in cui AudioFileProcessor inizierà la riproduzione. Se la modalità ripetizione è attiva, questo è il punto in cui la riproduzione ritorna se una nota è più lunga del campione tra i punti di inizio e fine.</translation>
    </message>
    <message>
        <source>Endpoint:</source>
        <translation>Punto di fine:</translation>
    </message>
    <message>
        <source>With this knob you can set the point where AudioFileProcessor should stop playing your sample. If you enable looping-mode, this is the point where AudioFileProcessor returns if a note is longer than the sample between the start and end-points.</source>
        <translation>Questa manopola regola il punto in cui AudioFileProcessor terminerà la riproduzione. Se la modalità ripetizione è attiva, questo è il punto in cui la riproduzione si ferma se una nota è più lunga del campione tra i punti di inizio e fine.</translation>
    </message>
    <message>
        <source>Continue sample playback across notes</source>
        <translation>Continua la ripetizione del campione tra le note</translation>
    </message>
    <message>
        <source>Enabling this option makes the sample continue playing across different notes - if you change pitch, or the note length stops before the end of the sample, then the next note played will continue where it left off. To reset the playback to the start of the sample, insert a note at the bottom of the keyboard (&lt; 20 Hz)</source>
        <translation>Attivando questa opzione, il campione audio viene riprodotto tra note differenti: se cambi l&apos;altezza, o la nota finisce prima del punto di fine del campione, allora la nota seguente riprodurrà il campione da dove si era fermata la precedente. Se invece si vuol far ripartire il campione dal punto d&apos;inizio, bisogna inserire una nota molto grave (&lt; 20 Hz)</translation>
    </message>
</context>
<context>
    <name>AudioFileProcessorWaveView</name>
    <message>
        <source>Sample length:</source>
        <translation>Lunghezza del campione:</translation>
    </message>
</context>
<context>
    <name>AudioJack</name>
    <message>
        <source>JACK client restarted</source>
        <translation>Il client JACK è ripartito</translation>
    </message>
    <message>
        <source>LMMS was kicked by JACK for some reason. Therefore the JACK backend of LMMS has been restarted. You will have to make manual connections again.</source>
        <translation>LMMS è stato kickato da JACK per qualche motivo. Quindi il collegamento JACK di LMMS è ripartito. Dovrai rifare le connessioni.</translation>
    </message>
    <message>
        <source>JACK server down</source>
        <translation>Il server JACK è down</translation>
    </message>
    <message>
        <source>The JACK server seems to have been shutdown and starting a new instance failed. Therefore LMMS is unable to proceed. You should save your project and restart JACK and LMMS.</source>
        <translation>Il server JACK sembra essere stato spento e non sono partite nuove istanze. Quindi LMMS non è in grado di procedere. Salva il progetto attivo e fai ripartire JACK ed LMMS.</translation>
    </message>
</context>
<context>
    <name>AudioJack::setupWidget</name>
    <message>
        <source>CLIENT-NAME</source>
        <translation>NOME DEL CLIENT</translation>
    </message>
    <message>
        <source>CHANNELS</source>
        <translation>CANALI</translation>
    </message>
</context>
<context>
    <name>AudioOss::setupWidget</name>
    <message>
        <source>DEVICE</source>
        <translation>PERIFERICA</translation>
    </message>
    <message>
        <source>CHANNELS</source>
        <translation>CANALI</translation>
    </message>
</context>
<context>
    <name>AudioPortAudio::setupWidget</name>
    <message>
        <source>BACKEND</source>
        <translation>USCITA POSTERIORE</translation>
    </message>
    <message>
        <source>DEVICE</source>
        <translation>PERIFERICA</translation>
    </message>
</context>
<context>
    <name>AudioPulseAudio::setupWidget</name>
    <message>
        <source>DEVICE</source>
        <translation>PERIFERICA</translation>
    </message>
    <message>
        <source>CHANNELS</source>
        <translation>CANALI</translation>
    </message>
</context>
<context>
    <name>AudioSdl::setupWidget</name>
    <message>
        <source>DEVICE</source>
        <translation>PERIFERICA</translation>
    </message>
</context>
<context>
    <name>AutomatableModel</name>
    <message>
        <source>&amp;Reset (%1%2)</source>
        <translation>&amp;Reimposta (%1%2)</translation>
    </message>
    <message>
        <source>&amp;Copy value (%1%2)</source>
        <translation>&amp;Copia valore (%1%2)</translation>
    </message>
    <message>
        <source>&amp;Paste value (%1%2)</source>
        <translation>&amp;Incolla valore (%1%2)</translation>
    </message>
    <message>
        <source>Edit song-global automation</source>
        <translation>Modifica l&apos;automazione globale della traccia</translation>
    </message>
    <message>
        <source>Connected to %1</source>
        <translation>Connesso a %1</translation>
    </message>
    <message>
        <source>Connected to controller</source>
        <translation>Connesso a un controller</translation>
    </message>
    <message>
        <source>Edit connection...</source>
        <translation>Modifica connessione...</translation>
    </message>
    <message>
        <source>Remove connection</source>
        <translation>Rimuovi connessione</translation>
    </message>
    <message>
        <source>Connect to controller...</source>
        <translation>Connetti a un controller...</translation>
    </message>
    <message>
        <source>Remove song-global automation</source>
        <translation>Rimuovi l&apos;automazione globale della traccia</translation>
    </message>
    <message>
        <source>Remove all linked controls</source>
        <translation>Rimuovi tutti i controlli collegati</translation>
    </message>
</context>
<context>
    <name>AutomationEditor</name>
    <message>
        <source>Play/pause current pattern (Space)</source>
        <translation>Riproduci/metti in pausa questo pattern (Barra Spaziatrice)</translation>
    </message>
    <message>
        <source>Stop playing of current pattern (Space)</source>
        <translation>Ferma la riproduzione di questo pattern (Barra Spaziatrice)</translation>
    </message>
    <message>
        <source>Click here if you want to play the current pattern. This is useful while editing it.  The pattern is automatically looped when the end is reached.</source>
        <translation>Cliccando qui si riproduce il pattern selezionato. Questo è utile mentre lo  si modifica. Il pattern viene automaticamente ripetuto quando finisce.</translation>
    </message>
    <message>
        <source>Click here if you want to stop playing of the current pattern.</source>
        <translation>Cliccando qui si ferma la riproduzione del pattern.</translation>
    </message>
    <message>
        <source>Draw mode (Shift+D)</source>
        <translation>Modalità disegno (Shift+D)</translation>
    </message>
    <message>
        <source>Erase mode (Shift+E)</source>
        <translation>Modalità cancella (Shift+E)</translation>
    </message>
    <message>
        <source>Click here and draw-mode will be activated. In this mode you can add and move single values.  This is the default mode which is used most of the time.  You can also press &apos;Shift+D&apos; on your keyboard to activate this mode.</source>
        <translation>Cliccando qui si attiva la modalità disegno. In questa modalità è possibile aggiungere e spostare singoli valori. Questa è la modalità predefinita, che viene usata la maggior parte del tempo. Questa modalità si attiva anche premendo la combinazione di tasti &apos;Shift+D&apos;.</translation>
    </message>
    <message>
        <source>Click here and erase-mode will be activated. In this mode you can erase single values. You can also press &apos;Shift+E&apos; on your keyboard to activate this mode.</source>
        <translation>Cliccando qui si attiva la modalità cancellazione. In questa modalità è possibile cancellare singoli valori. Questa modalità si attiva anche premendo la combinazione di tasti &apos;Shift+E&apos;.</translation>
    </message>
    <message>
        <source>Cut selected values (Ctrl+X)</source>
        <translation>Taglia i valori selezionati (Ctrl+X)</translation>
    </message>
    <message>
        <source>Copy selected values (Ctrl+C)</source>
        <translation>Copia i valori selezionati (Ctrl+C)</translation>
    </message>
    <message>
        <source>Paste values from clipboard (Ctrl+V)</source>
        <translation>Incolla i valori selezionati (Ctrl+V)</translation>
    </message>
    <message>
        <source>Click here and selected values will be cut into the clipboard.  You can paste them anywhere in any pattern by clicking on the paste button.</source>
        <translation>Cliccando qui i valori selezionati vengono spostati nella clipboard. È possibile incollarli ovunque, in qualsiasi pattern, cliccando il pulsante Incolla.</translation>
    </message>
    <message>
        <source>Click here and selected values will be copied into the clipboard.  You can paste them anywhere in any pattern by clicking on the paste button.</source>
        <translation>Cliccando qui i valori selezionati vengono copiati della clipboard. È possibile incollarli ovunque, in qualsiasi pattern, cliccando il pulsante Incolla.</translation>
    </message>
    <message>
        <source>Click here and the values from the clipboard will be pasted at the first visible measure.</source>
        <translation>Cliccando qui i valori nella clipboard vengono incollati alla prima battuta visibile.</translation>
    </message>
    <message>
        <source>Automation Editor - no pattern</source>
        <translation>Editor dell&apos;automazione - nessun pattern</translation>
    </message>
    <message>
        <source>Automation Editor - %1</source>
        <translation>Editor dell&apos;automazione - %1</translation>
    </message>
    <message>
        <source>Please open an automation pattern with the context menu of a control!</source>
        <translation>È necessario aprire un pattern di automazione con il menu contestuale di un controllo!</translation>
    </message>
    <message>
        <source>Values copied</source>
        <translation>Valori copiati</translation>
    </message>
    <message>
        <source>All selected values were copied to the clipboard.</source>
        <translation>Tutti i valori sono stati copiati nella clipboard.</translation>
    </message>
    <message>
        <source>Discrete progression</source>
        <translation>Progressione discreta</translation>
    </message>
    <message>
        <source>Linear progression</source>
        <translation>Progressione lineare</translation>
    </message>
    <message>
        <source>Cubic Hermite progression</source>
        <translation>Progressione a cubica di Hermite</translation>
    </message>
    <message>
        <source>Tension: </source>
        <translation>tensione: </translation>
    </message>
    <message>
        <source>Click here to choose discrete progressions for this automation pattern.  The value of the connected object will remain constant between control points and be set immediately to the new value when each control point is reached.</source>
        <translation>Clicca qui per scegliere il metodo di progressione discreta per questo pattern di automazione. Il valore della variabile connessa rimarrà costante tra i punti disegnati, cambierà immediatamente non appena raggiunto ogni punto.</translation>
    </message>
    <message>
        <source>Click here to choose linear progressions for this automation pattern.  The value of the connected object will change at a steady rate over time between control points to reach the correct value at each control point without a sudden change.</source>
        <translation>Clicca qui per scegliere il metodo di progressione lineare per questo pattern di automazione. Il valore della variabile connessa cambierà in modo costante nel tempo tra i punti disegnati per arrivare al valore di ogni punto senza cambiamenti bruschi.</translation>
    </message>
    <message>
        <source>Click here to choose cubic hermite progressions for this automation pattern.  The value of the connected object will change in a smooth curve and ease in to the peaks and valleys.</source>
        <translation>Clicca qui per scegliere il metodo di progressione a cubica di Hermite per questo pattern di automazione. Il valore della variabile connessa cambierà seguendo una curva morbida.</translation>
    </message>
    <message>
        <source>Tension value for spline</source>
        <translation>Valore di tensione per la spline</translation>
    </message>
    <message>
        <source>A higher tension value may make a smoother curve but overshoot some values.  A low tension value will cause the slope of the curve to level off at each control point.</source>
        <translation>Un&apos;alta tensione può creare una curva più morbida, ma potrebbe non eseguire alcuni valori con precisione.  Una bassa tensione farà stabilizzare la pendenza della curva verso i valori dei punti disegnati.</translation>
    </message>
</context>
<context>
    <name>AutomationPattern</name>
    <message>
        <source>Drag a control while pressing &lt;Ctrl&gt;</source>
        <translation>Trascina un controllo tenendo premuto &lt;Ctrl&gt;</translation>
    </message>
</context>
<context>
    <name>AutomationPatternView</name>
    <message>
        <source>double-click to open this pattern in automation editor</source>
        <translation>Fai doppio-click per disegnare questo pattern di automazione</translation>
    </message>
    <message>
        <source>Open in Automation editor</source>
        <translation>Apri nell&apos;editor dell&apos;Automazione</translation>
    </message>
    <message>
        <source>Clear</source>
        <translation>Pulisci</translation>
    </message>
    <message>
        <source>Reset name</source>
        <translation>Reimposta nome</translation>
    </message>
    <message>
        <source>Change name</source>
        <translation>Rinomina</translation>
    </message>
    <message>
        <source>%1 Connections</source>
        <translation>%1 connessioni</translation>
    </message>
    <message>
        <source>Disconnect &quot;%1&quot;</source>
        <translation>Disconnetti &quot;%1&quot;</translation>
    </message>
</context>
<context>
    <name>AutomationTrack</name>
    <message>
        <source>Automation track</source>
        <translation>Traccia di automazione</translation>
    </message>
</context>
<context>
    <name>Controller</name>
    <message>
        <source>Controller %1</source>
        <translation>Controller %1</translation>
    </message>
</context>
<context>
    <name>ControllerConnectionDialog</name>
    <message>
        <source>Connection Settings</source>
        <translation>Impostazioni connessione</translation>
    </message>
    <message>
        <source>MIDI CONTROLLER</source>
        <translation>CONTROLLER MIDI</translation>
    </message>
    <message>
        <source>Input channel</source>
        <translation>Canale di ingresso</translation>
    </message>
    <message>
        <source>CHANNEL</source>
        <translation>CANALE</translation>
    </message>
    <message>
        <source>Input controller</source>
        <translation>Controller di  input</translation>
    </message>
    <message>
        <source>CONTROLLER</source>
        <translation>CONTROLLER</translation>
    </message>
    <message>
        <source>Auto Detect</source>
        <translation>Rilevamento automatico</translation>
    </message>
    <message>
        <source>MIDI-devices to receive MIDI-events from</source>
        <translation>Le periferiche MIDI ricevono eventi MIDI da</translation>
    </message>
    <message>
        <source>USER CONTROLLER</source>
        <translation>CONTROLLER PERSONALIZZATO</translation>
    </message>
    <message>
        <source>MAPPING FUNCTION</source>
        <translation>FUNZIONE DI MAPPATURA</translation>
    </message>
    <message>
        <source>OK</source>
        <translation>OK</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation>Annulla</translation>
    </message>
    <message>
        <source>LMMS</source>
        <translation>LMMS</translation>
    </message>
    <message>
        <source>Cycle Detected.</source>
        <translation>Ciclo rilevato.</translation>
    </message>
</context>
<context>
    <name>ControllerRackView</name>
    <message>
        <source>Controller Rack</source>
        <translation>Rack di Controller</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Aggiungi</translation>
    </message>
    <message>
        <source>Confirm Delete</source>
        <translation>Conferma eliminazione</translation>
    </message>
    <message>
        <source>Confirm delete? There are existing connection(s) associted with this controller. There is no way to undo.</source>
        <translation>Confermi l&apos;eliminazione? Ci sono connessioni associate a questo controller: non sarà possibile ripristinarle.</translation>
    </message>
</context>
<context>
    <name>ControllerView</name>
    <message>
        <source>Controls</source>
        <translation>Controlli</translation>
    </message>
    <message>
        <source>Controllers are able to automate the value of a knob, slider, and other controls.</source>
        <translation>I controller possono automatizzare il valore di una manopola, di uno slider e di altri controlli.</translation>
    </message>
    <message>
        <source>Rename controller</source>
        <translation>Rinomina controller</translation>
    </message>
    <message>
        <source>Enter the new name for this controller</source>
        <translation>Inserire il nuovo nome di questo controller</translation>
    </message>
    <message>
        <source>&amp;Remove this plugin</source>
        <translation>&amp;Elimina questo plugin</translation>
    </message>
    <message>
        <source>&amp;Help</source>
        <translation>&amp;Aiuto</translation>
    </message>
</context>
<context>
    <name>Effect</name>
    <message>
        <source>Effect enabled</source>
        <translation>Effetto attivo</translation>
    </message>
    <message>
        <source>Wet/Dry mix</source>
        <translation>Bilanciamento Wet/Dry</translation>
    </message>
    <message>
        <source>Gate</source>
        <translation>Gate</translation>
    </message>
    <message>
        <source>Decay</source>
        <translation>Decadimento</translation>
    </message>
</context>
<context>
    <name>EffectChain</name>
    <message>
        <source>Effects enabled</source>
        <translation>Effetti abilitati</translation>
    </message>
</context>
<context>
    <name>EffectRackView</name>
    <message>
        <source>EFFECTS CHAIN</source>
        <translation>CATENA DI EFFETTI</translation>
    </message>
    <message>
        <source>Add effect</source>
        <translation>Aggiungi effetto</translation>
    </message>
</context>
<context>
    <name>EffectSelectDialog</name>
    <message>
        <source>Add effect</source>
        <translation>Aggiungi effetto</translation>
    </message>
    <message>
        <source>Plugin description</source>
        <translation>Descrizione Plugin</translation>
    </message>
</context>
<context>
    <name>EffectView</name>
    <message>
        <source>Toggles the effect on or off.</source>
        <translation>Abilita o disabilita l&apos;effetto.</translation>
    </message>
    <message>
        <source>On/Off</source>
        <translation>On/Off</translation>
    </message>
    <message>
        <source>W/D</source>
        <translation>W/D</translation>
    </message>
    <message>
        <source>Wet Level:</source>
        <translation>Livello del segnale modificato:</translation>
    </message>
    <message>
        <source>The Wet/Dry knob sets the ratio between the input signal and the effect signal that forms the output.</source>
        <translation>La manopola Wet/Dry imposta il rapporto tra il segnale in ingresso e la quantità di effetto udibile in uscita.</translation>
    </message>
    <message>
        <source>DECAY</source>
        <translation>DECAY</translation>
    </message>
    <message>
        <source>Time:</source>
        <translation>Tempo:</translation>
    </message>
    <message>
        <source>The Decay knob controls how many buffers of silence must pass before the plugin stops processing.  Smaller values will reduce the CPU overhead but run the risk of clipping the tail on delay and reverb effects.</source>
        <translation>La manopola Decadimento controlla quanto silenzio deve esserci prima che il plugin si fermi. Valori più piccoli riducono il carico del processore ma rischiano di troncare la parte finale degli effetti di delay.</translation>
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
        <translation>La manopola Gate controlla il livello del segnale che è considerato &apos;silenzio&apos; per decidere quando smettere di processare i segnali.</translation>
    </message>
    <message>
        <source>Controls</source>
        <translation>Controlli</translation>
    </message>
    <message>
        <source>Effect plugins function as a chained series of effects where the signal will be processed from top to bottom.

The On/Off switch allows you to bypass a given plugin at any point in time.

The Wet/Dry knob controls the balance between the input signal and the effected signal that is the resulting output from the effect.  The input for the stage is the output from the previous stage. So, the &apos;dry&apos; signal for effects lower in the chain contains all of the previous effects.

The Decay knob controls how long the signal will continue to be processed after the notes have been released.  The effect will stop processing signals when the volume has dropped below a given threshold for a given length of time.  This knob sets the &apos;given length of time&apos;.  Longer times will require more CPU, so this number should be set low for most effects.  It needs to be bumped up for effects that produce lengthy periods of silence, e.g. delays.

The Gate knob controls the &apos;given threshold&apos; for the effect&apos;s auto shutdown.  The clock for the &apos;given length of time&apos; will begin as soon as the processed signal level drops below the level specified with this knob.

The Controls button opens a dialog for editing the effect&apos;s parameters.

Right clicking will bring up a context menu where you can change the order in which the effects are processed or delete an effect altogether.</source>
        <translation>I plugin di effetti funzionano come una catena di effetti sottoposti al segnale dall&apos;alto verso il basso.

L&apos;interruttore On/Off permette di saltare un certo plugin in qualsiasi momento.

La manopola Wet/Dry controlla il bilanciamento tra il segnale in ingresso e quello processato che viene emesso dall&apos;effetto. L&apos;ingresso di ogni stadio è costituito dall&apos;uscita dello stadio precedente, così il segnale &apos;dry&apos; degli effetti sottostanti contiene tutti i precedenti effetti.

La manopola Decadimento controlla il tempo per cui il segnale viene processato dopo che le note sono state rilasciate. L&apos;effetto smette di processare il segnale quando questo scende sotto una certa soglia per un certo periodo ti tempo. La manopola imposta questo periodo di tempo. Tempi maggiori richiedono una maggiore potenza del processore, quindi il tempo dovrebbe rimanere basso per la maggior parte degli effetti. È necessario aumentarlo per effetti che producono lunghi periodi di silenzio, ad es. i delay.

La manopola Gate regola la soglia sotto la quale l&apos;effetto smette di processare il segnale. Il conteggio del tempo di silenzio necessario inizia non appena il sengale processato scende sotto la soglia specificata.

Il pulsante Controlli apre una finestra per modificare i parametri dell&apos;effetto.

Con il click destro si apre un menu conestuale che permette di cambiare l&apos;ordine degli effetti o di eliminarli.</translation>
    </message>
    <message>
        <source>Move &amp;up</source>
        <translation>Sposta verso l&apos;&amp;alto</translation>
    </message>
    <message>
        <source>Move &amp;down</source>
        <translation>Sposta verso il &amp;basso</translation>
    </message>
    <message>
        <source>&amp;Remove this plugin</source>
        <translation>&amp;Elimina questo plugin</translation>
    </message>
    <message>
        <source>&amp;Help</source>
        <translation>&amp;Aiuto</translation>
    </message>
</context>
<context>
    <name>EnvelopeAndLfoParameters</name>
    <message>
        <source>Predelay</source>
        <translation>Ritardo iniziale</translation>
    </message>
    <message>
        <source>Attack</source>
        <translation>Attacco</translation>
    </message>
    <message>
        <source>Hold</source>
        <translation>Mantenimento</translation>
    </message>
    <message>
        <source>Decay</source>
        <translation>Decadimento</translation>
    </message>
    <message>
        <source>Sustain</source>
        <translation>Sostegno</translation>
    </message>
    <message>
        <source>Release</source>
        <translation>Rilascio</translation>
    </message>
    <message>
        <source>Modulation</source>
        <translation>Modulazione</translation>
    </message>
    <message>
        <source>LFO Predelay</source>
        <translation>Ritardo iniziale dell&apos;LFO</translation>
    </message>
    <message>
        <source>LFO Attack</source>
        <translation>Attacco dell&apos;LFO</translation>
    </message>
    <message>
        <source>LFO speed</source>
        <translation>Velocità dell&apos;LFO</translation>
    </message>
    <message>
        <source>LFO Modulation</source>
        <translation>Modulazione dell&apos;LFO</translation>
    </message>
    <message>
        <source>LFO Wave Shape</source>
        <translation>Forma d&apos;onda dell&apos;LFO</translation>
    </message>
    <message>
        <source>Freq x 100</source>
        <translation>Freq x 100</translation>
    </message>
    <message>
        <source>Modulate Env-Amount</source>
        <translation>Modula la quantità di Env</translation>
    </message>
</context>
<context>
    <name>EnvelopeAndLfoView</name>
    <message>
        <source>DEL</source>
        <translation>RIT</translation>
    </message>
    <message>
        <source>Predelay:</source>
        <translation>Ritardo iniziale:</translation>
    </message>
    <message>
        <source>Use this knob for setting predelay of the current envelope. The bigger this value the longer the time before start of actual envelope.</source>
        <translation>Questa manopola imposta il ritardo iniziale dell&apos;envelope selezionato. Più grande è questo valore più tempo passerà prima che l&apos;envelope effettivo inizi.</translation>
    </message>
    <message>
        <source>ATT</source>
        <translation>ATT</translation>
    </message>
    <message>
        <source>Attack:</source>
        <translation>Attacco:</translation>
    </message>
    <message>
        <source>Use this knob for setting attack-time of the current envelope. The bigger this value the longer the envelope needs to increase to attack-level. Choose a small value for instruments like pianos and a big value for strings.</source>
        <translation>Questa manopola imposta il tempo di attacco dell&apos;envelope selezionato. Più grande è questo valore più tempo passa prima di raggiungere il livello di attacco. Scegliere un valore contenuto per strumenti come pianoforti e un valore grande per gli archi.</translation>
    </message>
    <message>
        <source>HOLD</source>
        <translation>MANT</translation>
    </message>
    <message>
        <source>Hold:</source>
        <translation>Mantenimento:</translation>
    </message>
    <message>
        <source>Use this knob for setting hold-time of the current envelope. The bigger this value the longer the envelope holds attack-level before it begins to decrease to sustain-level.</source>
        <translation>Questa manopola imposta il tempo di mantenimento dell&apos;envelope selezionato. Più grande è questo valore più a lungo l&apos;envelope manterrà il livello di attacco prima di cominciare a scendere verso il livello di sostegno.</translation>
    </message>
    <message>
        <source>DEC</source>
        <translation>DEC</translation>
    </message>
    <message>
        <source>Decay:</source>
        <translation>Decadimento:</translation>
    </message>
    <message>
        <source>Use this knob for setting decay-time of the current envelope. The bigger this value the longer the envelope needs to decrease from attack-level to sustain-level. Choose a small value for instruments like pianos.</source>
        <translation>Questa manopola imposta il tempo di decdimento dell&apos;envelope selezionato. Più grande è questo valore più lentamente l&apos;envelope decadrà dal livello di attacco a quello di sustain. Scegliere un valore piccolo per strumenti come i pianoforti.</translation>
    </message>
    <message>
        <source>SUST</source>
        <translation>SOST</translation>
    </message>
    <message>
        <source>Sustain:</source>
        <translation>Sostegno:</translation>
    </message>
    <message>
        <source>Use this knob for setting sustain-level of the current envelope. The bigger this value the higher the level on which the envelope stays before going down to zero.</source>
        <translation>Questa manopola imposta il livello di sostegno dell&apos;envelope selezionato. Più grande è questo valore più sarà alto il livello che l&apos;envelope manterrà prima di scendere a zero.</translation>
    </message>
    <message>
        <source>REL</source>
        <translation>RIL</translation>
    </message>
    <message>
        <source>Release:</source>
        <translation>Rilascio:</translation>
    </message>
    <message>
        <source>Use this knob for setting release-time of the current envelope. The bigger this value the longer the envelope needs to decrease from sustain-level to zero. Choose a big value for soft instruments like strings.</source>
        <translation>Questa manopola imposta il tempo di rilascio dell&apos;anvelope selezionato. Più grande è questo valore più tempo l&apos;envelope impiegherà per scendere dal livello di sustain a zero. Scegliere un valore grande per strumenti dal suono morbido, come gli archi.</translation>
    </message>
    <message>
        <source>AMT</source>
        <translation>Q.TÀ</translation>
    </message>
    <message>
        <source>Modulation amount:</source>
        <translation>Quantità di modulazione:</translation>
    </message>
    <message>
        <source>Use this knob for setting modulation amount of the current envelope. The bigger this value the more the according size (e.g. volume or cutoff-frequency) will be influenced by this envelope.</source>
        <translation>Questa manopola imposta la quantità di modulazione dell&apos;envelope selezionato. Più grande è questo valore, più la grandezza corrispondente (ad es. il volume o la frequenza di taglio) sarà influenzata da questo envelope.</translation>
    </message>
    <message>
        <source>LFO predelay:</source>
        <translation>Ritardo iniziale dell&apos;LFO:</translation>
    </message>
    <message>
        <source>Use this knob for setting predelay-time of the current LFO. The bigger this value the the time until the LFO starts to oscillate.</source>
        <translation>Questa manopola imposta il ritardo iniziale dell&apos;LFO selezionato. Più grande è questo valore più tempo passerà prima che l&apos;LFO inizi a oscillare.</translation>
    </message>
    <message>
        <source>LFO- attack:</source>
        <translation>Attacco dell&apos;LFO:</translation>
    </message>
    <message>
        <source>Use this knob for setting attack-time of the current LFO. The bigger this value the longer the LFO needs to increase its amplitude to maximum.</source>
        <translation>Questa manopola imposta il tempo di attaco dell&apos;LFO selezionato. Più grande è questo valore più tempo l&apos;LFO impiegherà per raggiungere la massima ampiezza.</translation>
    </message>
    <message>
        <source>SPD</source>
        <translation>VEL</translation>
    </message>
    <message>
        <source>LFO speed:</source>
        <translation>Velocità dell&apos;LFO:</translation>
    </message>
    <message>
        <source>Use this knob for setting speed of the current LFO. The bigger this value the faster the LFO oscillates and the faster will be your effect.</source>
        <translation>Questa manopola imposta la velocità dell&apos;LFO selezionato. Più grande è questo valore più velocemente l&apos;LFO oscillerà e più veloce sarà l&apos;effetto.</translation>
    </message>
    <message>
        <source>Use this knob for setting modulation amount of the current LFO. The bigger this value the more the selected size (e.g. volume or cutoff-frequency) will be influenced by this LFO.</source>
        <translation>Questa manopola imposta la quantità di modulazione dell&apos;LFO selezionato. Più grande è questo valore più la grandezza selezionata (ad es. il volume o la frequenza di taglio) sarà influenzata da questo LFO.</translation>
    </message>
    <message>
        <source>Click here for a sine-wave.</source>
        <translation>Cliccando qui si ha un&apos;onda sinusoidale.</translation>
    </message>
    <message>
        <source>Click here for a triangle-wave.</source>
        <translation>Cliccando qui si ottiene un&apos;onda triangolare.</translation>
    </message>
    <message>
        <source>Click here for a saw-wave for current.</source>
        <translation>Cliccando qui si ha un&apos;onda a dente di sega.</translation>
    </message>
    <message>
        <source>Click here for a square-wave.</source>
        <translation>Cliccando qui si ottiene un&apos;onda quadra.</translation>
    </message>
    <message>
        <source>Click here for a user-defined wave. Afterwards, drag an according sample-file onto the LFO graph.</source>
        <translation>Cliccando qui è possibile definire una forma d&apos;onda personalizzata. Successivamente è possibile trascinare un file di campione nel grafico dell&apos;LFO.</translation>
    </message>
    <message>
        <source>FREQ x 100</source>
        <translation>FREQ x 100</translation>
    </message>
    <message>
        <source>Click here if the frequency of this LFO should be multiplied by 100.</source>
        <translation>Cliccando qui la frequenza di questo LFO viene moltiplicata per 100.</translation>
    </message>
    <message>
        <source>multiply LFO-frequency by 100</source>
        <translation>moltiplica la frequenza dell&apos;LFO per 100</translation>
    </message>
    <message>
        <source>MODULATE ENV-AMOUNT</source>
        <translation>MODULA LA QUANTITA&apos; DI ENVELOPE</translation>
    </message>
    <message>
        <source>Click here to make the envelope-amount controlled by this LFO.</source>
        <translation>Cliccando qui questo LFO controlla la quantità di envelope.</translation>
    </message>
    <message>
        <source>control envelope-amount by this LFO</source>
        <translation>controlla la quantità di envelope con questo LFO</translation>
    </message>
    <message>
        <source>ms/LFO:</source>
        <translation>ms/LFO:</translation>
    </message>
    <message>
        <source>Hint</source>
        <translation>Suggerimento</translation>
    </message>
    <message>
        <source>Drag a sample from somewhere and drop it in this window.</source>
        <translation>È possibile trascinare un campione in questa finestra.</translation>
    </message>
</context>
<context>
    <name>ExportProjectDialog</name>
    <message>
        <source>Export project</source>
        <translation>Esporta il progetto</translation>
    </message>
    <message>
        <source>Output</source>
        <translation>Codifica</translation>
    </message>
    <message>
        <source>File format:</source>
        <translation>Formato file:</translation>
    </message>
    <message>
        <source>Samplerate:</source>
        <translation>Frequenza di campionamento:</translation>
    </message>
    <message>
        <source>44100 Hz</source>
        <translation></translation>
    </message>
    <message>
        <source>48000 Hz</source>
        <translation></translation>
    </message>
    <message>
        <source>88200 Hz</source>
        <translation></translation>
    </message>
    <message>
        <source>96000 Hz</source>
        <translation></translation>
    </message>
    <message>
        <source>192000 Hz</source>
        <translation></translation>
    </message>
    <message>
        <source>Bitrate:</source>
        <translation>Bitrate:</translation>
    </message>
    <message>
        <source>64 KBit/s</source>
        <translation></translation>
    </message>
    <message>
        <source>128 KBit/s</source>
        <translation></translation>
    </message>
    <message>
        <source>160 KBit/s</source>
        <translation></translation>
    </message>
    <message>
        <source>192 KBit/s</source>
        <translation></translation>
    </message>
    <message>
        <source>256 KBit/s</source>
        <translation></translation>
    </message>
    <message>
        <source>320 KBit/s</source>
        <translation></translation>
    </message>
    <message>
        <source>Depth:</source>
        <translation>Risoluzione Bit:</translation>
    </message>
    <message>
        <source>16 Bit Integer</source>
        <translation></translation>
    </message>
    <message>
        <source>32 Bit Float</source>
        <translation></translation>
    </message>
    <message>
        <source>Please note that not all of the parameters above apply for all file formats.</source>
        <translation>Non tutti i parametri si applicano nella creazione di tutti i formati.</translation>
    </message>
    <message>
        <source>Quality settings</source>
        <translation>Impostazioni qualità</translation>
    </message>
    <message>
        <source>Interpolation:</source>
        <translation>Interpolazione:</translation>
    </message>
    <message>
        <source>Zero Order Hold</source>
        <translation></translation>
    </message>
    <message>
        <source>Sinc Fastest</source>
        <translation></translation>
    </message>
    <message>
        <source>Sinc Medium (recommended)</source>
        <translation>Sinc Medium (suggerito)</translation>
    </message>
    <message>
        <source>Sinc Best (very slow!)</source>
        <translation>Sinc Best (molto lento!)</translation>
    </message>
    <message>
        <source>Oversampling (use with care!):</source>
        <translation>Oversampling (usare con cura!):</translation>
    </message>
    <message>
        <source>1x (None)</source>
        <translation>1x (Nessuna)</translation>
    </message>
    <message>
        <source>2x</source>
        <translation></translation>
    </message>
    <message>
        <source>4x</source>
        <translation></translation>
    </message>
    <message>
        <source>8x</source>
        <translation></translation>
    </message>
    <message>
        <source>Sample-exact controllers</source>
        <translation></translation>
    </message>
    <message>
        <source>Alias-free oscillators</source>
        <translation></translation>
    </message>
    <message>
        <source>Start</source>
        <translation>Inizia</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation>Annulla</translation>
    </message>
    <message>
        <source>Export as loop (remove end silence)</source>
        <translation>Esporta come loop (rimuove il silenzio finale)</translation>
    </message>
</context>
<context>
    <name>FxMixer</name>
    <message>
        <source>Master</source>
        <translation>Master</translation>
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
        <translation>Rinomina il canale FX</translation>
    </message>
    <message>
        <source>Enter the new name for this FX channel</source>
        <translation>Inserire il nuovo nome di questo canale FX</translation>
    </message>
    <message>
        <source>FX-Mixer</source>
        <translation>Mixer FX</translation>
    </message>
    <message>
        <source>FX Fader %1</source>
        <translation>Fader FX %1</translation>
    </message>
    <message>
        <source>Mute</source>
        <translation>Muto</translation>
    </message>
    <message>
        <source>Mute this FX channel</source>
        <translation>Silenzia questo canale FX</translation>
    </message>
</context>
<context>
    <name>InstrumentFunctionArpeggio</name>
    <message>
        <source>Arpeggio</source>
        <translation>Arpeggio</translation>
    </message>
    <message>
        <source>Arpeggio type</source>
        <translation>Tipo di arpeggio</translation>
    </message>
    <message>
        <source>Arpeggio range</source>
        <translation>Ampiezza dell&apos;arpeggio</translation>
    </message>
    <message>
        <source>Arpeggio time</source>
        <translation>Tempo dell&apos;arpeggio</translation>
    </message>
    <message>
        <source>Arpeggio gate</source>
        <translation>Gate dell&apos;arpeggio</translation>
    </message>
    <message>
        <source>Arpeggio direction</source>
        <translation>Direzione dell&apos;arpeggio</translation>
    </message>
    <message>
        <source>Arpeggio mode</source>
        <translation>Modo dell&apos;arpeggio</translation>
    </message>
    <message>
        <source>Up</source>
        <translation>Su</translation>
    </message>
    <message>
        <source>Down</source>
        <translation>Giù</translation>
    </message>
    <message>
        <source>Up and down</source>
        <translation>Su e giù</translation>
    </message>
    <message>
        <source>Random</source>
        <translation>Casuale</translation>
    </message>
    <message>
        <source>Free</source>
        <translation>Libero</translation>
    </message>
    <message>
        <source>Sort</source>
        <translation>Ordinato</translation>
    </message>
    <message>
        <source>Sync</source>
        <translation>Sincronizzato</translation>
    </message>
</context>
<context>
    <name>InstrumentFunctionArpeggioView</name>
    <message>
        <source>ARPEGGIO</source>
        <translation>ARPEGGIO</translation>
    </message>
    <message>
        <source>An arpeggio is a method playing (especially plucked) instruments, which makes the music much livelier. The strings of such instruments (e.g. harps) are plucked like chords. The only difference is that this is done in a sequential order, so the notes are not played at the same time. Typical arpeggios are major or minor triads, but there are a lot of other possible chords, you can select.</source>
        <translation>Un arpeggio è un modo di suonare alcuni strumenti (pizzicati in particolare), che rende la musica più viva. Le corde di tali strumenti (ad es. un&apos;arpa) vengono pizzicate come accordi. L&apos;unica differenza è che ciò viene fatto in ordine sequenziale, in modo che le note non vengano suonate allo stesso tempo. Arpeggi tipici sono quelli sulle triadi maggiore o minore, ma ci sono molte altre possibilità tra le quali si può scegliere.</translation>
    </message>
    <message>
        <source>RANGE</source>
        <translation>AMPIEZZA</translation>
    </message>
    <message>
        <source>Arpeggio range:</source>
        <translation>Ampiezza dell&apos;arpeggio:</translation>
    </message>
    <message>
        <source>octave(s)</source>
        <translation>ottava(e)</translation>
    </message>
    <message>
        <source>Use this knob for setting the arpeggio range in octaves. The selected arpeggio will be played within specified number of octaves.</source>
        <translation>Questa manopola imposta l&apos;ampiezza in ottave dell&apos;arpeggio. L&apos;arpeggio selezionato verrà suonato all&apos;interno del numero di ottave impostato.</translation>
    </message>
    <message>
        <source>TIME</source>
        <translation>TEMPO</translation>
    </message>
    <message>
        <source>Arpeggio time:</source>
        <translation>Tempo dell&apos;arpeggio:</translation>
    </message>
    <message>
        <source>ms</source>
        <translation>ms</translation>
    </message>
    <message>
        <source>Use this knob for setting the arpeggio time in milliseconds. The arpeggio time specifies how long each arpeggio-tone should be played.</source>
        <translation>Questa manopola imposta l&apos;ampiezza dell&apos;arpeggio in millisecondi. Il tempo dell&apos;arpeggio specifica per quanto tempo ogni nota dell&apos;arpeggio deve essere eseguita.</translation>
    </message>
    <message>
        <source>GATE</source>
        <translation>GATE</translation>
    </message>
    <message>
        <source>Arpeggio gate:</source>
        <translation>Gate dell&apos;arpeggio:</translation>
    </message>
    <message>
        <source>%</source>
        <translation>%</translation>
    </message>
    <message>
        <source>Use this knob for setting the arpeggio gate. The arpeggio gate specifies the percent of a whole arpeggio-tone that should be played. With this you can make cool staccato arpeggios.</source>
        <translation>Questa manopola imposta il gate dell&apos;arpeggio. Il gate dell&apos;arpeggio specifica la percentuale di ogni nota che deve essere eseguita. In questo modo si possono creare arpeggi particolari, con le note staccate.</translation>
    </message>
    <message>
        <source>Chord:</source>
        <translation>Tipo di arpeggio:</translation>
    </message>
    <message>
        <source>Direction:</source>
        <translation>Direzione:</translation>
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
        <translation>ottava</translation>
    </message>
    <message>
        <source>Major</source>
        <translation>Maggiore</translation>
    </message>
    <message>
        <source>Majb5</source>
        <translation>Majb5</translation>
    </message>
    <message>
        <source>minor</source>
        <translation>minore</translation>
    </message>
    <message>
        <source>minb5</source>
        <translation>minb5</translation>
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
        <translation>triade</translation>
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
        <translation>6add9</translation>
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
        <translation>Minore armonica</translation>
    </message>
    <message>
        <source>Melodic minor</source>
        <translation>Minore melodica</translation>
    </message>
    <message>
        <source>Whole tone</source>
        <translation>Toni interi</translation>
    </message>
    <message>
        <source>Diminished</source>
        <translation>Diminuita</translation>
    </message>
    <message>
        <source>Major pentatonic</source>
        <translation>Pentatonica maggiore</translation>
    </message>
    <message>
        <source>Minor pentatonic</source>
        <translation>Pentatonica minore</translation>
    </message>
    <message>
        <source>Jap in sen</source>
        <translation>Jap in sen</translation>
    </message>
    <message>
        <source>Major bebop</source>
        <translation>Bebop maggiore</translation>
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
        <translation>Araba</translation>
    </message>
    <message>
        <source>Enigmatic</source>
        <translation>Enigmatica</translation>
    </message>
    <message>
        <source>Neopolitan</source>
        <translation>Napoletana</translation>
    </message>
    <message>
        <source>Neopolitan minor</source>
        <translation>Napoletana minore</translation>
    </message>
    <message>
        <source>Hungarian minor</source>
        <translation>Ungherese minore</translation>
    </message>
    <message>
        <source>Dorian</source>
        <translation>Dorica</translation>
    </message>
    <message>
        <source>Phrygolydian</source>
        <translation>Phrygolydian</translation>
    </message>
    <message>
        <source>Lydian</source>
        <translation>Lidia</translation>
    </message>
    <message>
        <source>Mixolydian</source>
        <translation>Misolidia</translation>
    </message>
    <message>
        <source>Aeolian</source>
        <translation>Eolia</translation>
    </message>
    <message>
        <source>Locrian</source>
        <translation>Locria</translation>
    </message>
    <message>
        <source>Chords</source>
        <translation>Accordi</translation>
    </message>
    <message>
        <source>Chord type</source>
        <translation>Tipo di accordo</translation>
    </message>
    <message>
        <source>Chord range</source>
        <translation>Ampiezza dell&apos;accordo</translation>
    </message>
    <message>
        <source>Minor</source>
        <translation>Minore</translation>
    </message>
</context>
<context>
    <name>InstrumentFunctionNoteStackingView</name>
    <message>
        <source>RANGE</source>
        <translation>AMPIEZZA</translation>
    </message>
    <message>
        <source>Chord range:</source>
        <translation>Ampiezza degli accordi:</translation>
    </message>
    <message>
        <source>octave(s)</source>
        <translation>ottava(e)</translation>
    </message>
    <message>
        <source>Use this knob for setting the chord range in octaves. The selected chord will be played within specified number of octaves.</source>
        <translation>Questa manopola imposta l&apos;ampiezza degli accordi in ottave. L&apos;accordo selezionato verrà eseguito all&apos;interno del numero di ottave impostato.</translation>
    </message>
    <message>
        <source>STACKING</source>
        <translation>ACCORDI</translation>
    </message>
    <message>
        <source>Chord:</source>
        <translation>Tipo di accordo:</translation>
    </message>
</context>
<context>
    <name>InstrumentMidiIOView</name>
    <message>
        <source>ENABLE MIDI INPUT</source>
        <translation>ABILITA INGRESSO MIDI</translation>
    </message>
    <message>
        <source>CHANNEL</source>
        <translation>CANALE</translation>
    </message>
    <message>
        <source>VELOCITY</source>
        <translation>VALOCITY</translation>
    </message>
    <message>
        <source>ENABLE MIDI OUTPUT</source>
        <translation>ABILITA USCITA MIDI</translation>
    </message>
    <message>
        <source>PROGRAM</source>
        <translation>PROGRAMMA</translation>
    </message>
    <message>
        <source>MIDI devices to receive MIDI events from</source>
        <translation>Periferica MIDI da cui ricevere segnali MIDi</translation>
    </message>
    <message>
        <source>MIDI devices to send MIDI events to</source>
        <translation>Periferica MIDI a cui mandare segnali MIDI</translation>
    </message>
    <message>
        <source>NOTE</source>
        <translation></translation>
    </message>
</context>
<context>
    <name>InstrumentSoundShaping</name>
    <message>
        <source>VOLUME</source>
        <translation>VOLUME</translation>
    </message>
    <message>
        <source>Volume</source>
        <translation>Volume</translation>
    </message>
    <message>
        <source>CUTOFF</source>
        <translation>CUTOFF</translation>
    </message>
    <message>
        <source>Cutoff frequency</source>
        <translation>Frequenza di taglio</translation>
    </message>
    <message>
        <source>RESO</source>
        <translation>RISO</translation>
    </message>
    <message>
        <source>Resonance</source>
        <translation>Risonanza</translation>
    </message>
    <message>
        <source>Envelopes/LFOs</source>
        <translation>Envelope/LFO</translation>
    </message>
    <message>
        <source>Filter type</source>
        <translation>Tipo di filtro</translation>
    </message>
    <message>
        <source>Q/Resonance</source>
        <translation>Q/Risonanza</translation>
    </message>
    <message>
        <source>LowPass</source>
        <translation>PassaBasso</translation>
    </message>
    <message>
        <source>HiPass</source>
        <translation>PassaAlto</translation>
    </message>
    <message>
        <source>BandPass csg</source>
        <translation>PassaBanda csg</translation>
    </message>
    <message>
        <source>BandPass czpg</source>
        <translation>PassaBanda czpg</translation>
    </message>
    <message>
        <source>Notch</source>
        <translation>Notch</translation>
    </message>
    <message>
        <source>Allpass</source>
        <translation>Passatutto</translation>
    </message>
    <message>
        <source>Moog</source>
        <translation>Moog</translation>
    </message>
    <message>
        <source>2x LowPass</source>
        <translation>PassaBasso 2x</translation>
    </message>
    <message>
        <source>RC LowPass 12dB</source>
        <translation>RC PassaBasso 12dB</translation>
    </message>
    <message>
        <source>RC BandPass 12dB</source>
        <translation>RC PassaBanda 12dB</translation>
    </message>
    <message>
        <source>RC HighPass 12dB</source>
        <translation>RC PassaAlto 12dB</translation>
    </message>
    <message>
        <source>RC LowPass 24dB</source>
        <translation>RC PassaBasso 24dB</translation>
    </message>
    <message>
        <source>RC BandPass 24dB</source>
        <translation>RC PassaBanda 24dB</translation>
    </message>
    <message>
        <source>RC HighPass 24dB</source>
        <translation>RC PassaAlto 24dB</translation>
    </message>
    <message>
        <source>Vocal Formant Filter</source>
        <translation>Filtro a Formante di Voce</translation>
    </message>
</context>
<context>
    <name>InstrumentSoundShapingView</name>
    <message>
        <source>TARGET</source>
        <translation>OBIETTIVO</translation>
    </message>
    <message>
        <source>These tabs contain envelopes. They&apos;re very important for modifying a sound, in that they are almost always necessary for substractive synthesis. For example if you have a volume envelope, you can set when the sound should have a specific volume. If you want to create some soft strings then your sound has to fade in and out very softly. This can be done by setting large attack and release times. It&apos;s the same for other envelope targets like panning, cutoff frequency for the used filter and so on. Just monkey around with it! You can really make cool sounds out of a saw-wave with just some envelopes...!</source>
        <translation>Queste schede contengono envelopes. Sono molto importanti per modificare i suoni, senza contare che sono quasi sempre necessarie per la sintesi sottrattiva. Per esempio se si usa un envelope per il volume, si può impostare quando un suono avrà un certo volume. Si potrebbero voler creare archi dal suono morbido, allora il suono deve iniziare e finire in modo molto morbido; ciò si ottiene impostando tempi di attacco e di rilascio ampi. Lo stesso vale per le altre funzioni degli envelope, come il panning, la frequenza di taglio dei filtri e così via. Bisogna semplicemente giocarci un po&apos;! Si possono ottenere suoni veramente interessanti a partire da un&apos;onda a dente di sega usando soltanto qualche envelope...!</translation>
    </message>
    <message>
        <source>FILTER</source>
        <translation>FILTRO</translation>
    </message>
    <message>
        <source>Here you can select the built-in filter you want to use for this instrument-track. Filters are very important for changing the characteristics of a sound.</source>
        <translation>Qui è possibile selezionare il filtro da usare per questa traccia. I filtri sono molto importanti per cambiare le caratteristiche del suono.</translation>
    </message>
    <message>
        <source>Hz</source>
        <translation>Hz</translation>
    </message>
    <message>
        <source>Use this knob for setting the cutoff frequency for the selected filter. The cutoff frequency specifies the frequency for cutting the signal by a filter. For example a lowpass-filter cuts all frequencies above the cutoff frequency. A highpass-filter cuts all frequencies below cutoff frequency, and so on...</source>
        <translation>Questa manopola imposta la frequenza di taglio del filtro. La frequenza di taglio specifica la frequenza a cui viene tagliato il segnate di un filtro. Per esempio un filtro passa-basso taglia tutte le frequenze sopra la frequenza di taglio, mentre un filtro passa-alto taglia tutte le frequenza al di sotto della frequenza di taglio e così via...</translation>
    </message>
    <message>
        <source>RESO</source>
        <translation>RISO</translation>
    </message>
    <message>
        <source>Resonance:</source>
        <translation>Risonanza:</translation>
    </message>
    <message>
        <source>Use this knob for setting Q/Resonance for the selected filter. Q/Resonance tells the filter how much it should amplify frequencies near Cutoff-frequency.</source>
        <translation>Questa manopola imposta il parametro (Q) per la risonanza del filtro selezionato. Il parametro per la risonanza specifica l&apos;ampiezza della campana di frequenze intorno alla frequenza di taglio che devono essere amplificate.</translation>
    </message>
    <message>
        <source>FREQ</source>
        <translation>FREQ</translation>
    </message>
    <message>
        <source>cutoff frequency:</source>
        <translation>Frequenza del cutoff:</translation>
    </message>
</context>
<context>
    <name>InstrumentTrack</name>
    <message>
        <source>unnamed_track</source>
        <translation>traccia_senza_nome</translation>
    </message>
    <message>
        <source>Volume</source>
        <translation>Volume</translation>
    </message>
    <message>
        <source>Panning</source>
        <translation>Panning</translation>
    </message>
    <message>
        <source>Pitch</source>
        <translation>Altezza</translation>
    </message>
    <message>
        <source>FX channel</source>
        <translation>Canale FX</translation>
    </message>
    <message>
        <source>Default preset</source>
        <translation>Impostazioni predefinite</translation>
    </message>
    <message>
        <source>With this knob you can set the volume of the opened channel.</source>
        <translation>Questa manopola imposta il volume del canale.</translation>
    </message>
    <message>
        <source>Base note</source>
        <translation>Nota base</translation>
    </message>
    <message>
        <source>Pitch range</source>
        <translation>Estenzione dell&apos;altezza</translation>
    </message>
</context>
<context>
    <name>InstrumentTrackView</name>
    <message>
        <source>Volume</source>
        <translation>Volume</translation>
    </message>
    <message>
        <source>Volume:</source>
        <translation>Volume:</translation>
    </message>
    <message>
        <source>VOL</source>
        <translation>VOL</translation>
    </message>
    <message>
        <source>Panning</source>
        <translation>Panning</translation>
    </message>
    <message>
        <source>Panning:</source>
        <translation>Panning:</translation>
    </message>
    <message>
        <source>PAN</source>
        <translation>PAN</translation>
    </message>
    <message>
        <source>MIDI</source>
        <translation>MIDI</translation>
    </message>
    <message>
        <source>Input</source>
        <translation>Ingresso</translation>
    </message>
    <message>
        <source>Output</source>
        <translation>Uscita</translation>
    </message>
</context>
<context>
    <name>InstrumentTrackWindow</name>
    <message>
        <source>GENERAL SETTINGS</source>
        <translation>IMPOSTAZIONI GENERALI</translation>
    </message>
    <message>
        <source>Click here, if you want to save current channel settings in a preset-file. Later you can load this preset by double-clicking it in the preset-browser.</source>
        <translation>Cliccando qui si salvano le attuali impostazioni del canale in un file. Sarà poi possibile ricaricare queste impostazioni facendo doppio-click su questo file nel navigatore dei preset.</translation>
    </message>
    <message>
        <source>Instrument volume</source>
        <translation>Volume dello strumento</translation>
    </message>
    <message>
        <source>Volume:</source>
        <translation>Volume:</translation>
    </message>
    <message>
        <source>VOL</source>
        <translation>VOL</translation>
    </message>
    <message>
        <source>Panning</source>
        <translation>Panning</translation>
    </message>
    <message>
        <source>Panning:</source>
        <translation>Panning:</translation>
    </message>
    <message>
        <source>PAN</source>
        <translation>PAN</translation>
    </message>
    <message>
        <source>Pitch</source>
        <translation>Altezza</translation>
    </message>
    <message>
        <source>Pitch:</source>
        <translation>Altezza:</translation>
    </message>
    <message>
        <source>cents</source>
        <translation>centesimi</translation>
    </message>
    <message>
        <source>PITCH</source>
        <translation>ALTEZZA</translation>
    </message>
    <message>
        <source>FX channel</source>
        <translation>Canale FX</translation>
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
        <translation>Salva il preset</translation>
    </message>
    <message>
        <source>XML preset file (*.xpf)</source>
        <translation>File di preset XML (*.xpf)</translation>
    </message>
    <message>
        <source>PLUGIN</source>
        <translation>PLUGIN</translation>
    </message>
    <message>
        <source>Save current channel settings in a preset-file</source>
        <translation>Salva le attuali impostazioni del canale in un file di preset</translation>
    </message>
    <message>
        <source>Pitch range (semitones)</source>
        <translation>Ampiezza dell&apos;altezza (in semitoni)</translation>
    </message>
    <message>
        <source>RANGE</source>
        <translation>AMPIEZZA</translation>
    </message>
</context>
<context>
    <name>LadspaControl</name>
    <message>
        <source>Link channels</source>
        <translation>Abbina i canali</translation>
    </message>
</context>
<context>
    <name>LadspaControlDialog</name>
    <message>
        <source>Link Channels</source>
        <translation>Canali abbinati</translation>
    </message>
    <message>
        <source>Channel </source>
        <translation>Canale</translation>
    </message>
</context>
<context>
    <name>LadspaControlView</name>
    <message>
        <source>Link channels</source>
        <translation>Abbina i canali</translation>
    </message>
    <message>
        <source>Value:</source>
        <translation>Valore:</translation>
    </message>
    <message>
        <source>Sorry, no help available.</source>
        <translation>Spiacente, nessun aiuto disponibile.</translation>
    </message>
</context>
<context>
    <name>LadspaEffect</name>
    <message>
        <source>Effect</source>
        <translation>Effetto</translation>
    </message>
    <message>
        <source>Unknown LADSPA plugin %1 requested.</source>
        <translation>Il plugin LADSPA  %1 richiesto è sconosciuto.</translation>
    </message>
</context>
<context>
    <name>LfoController</name>
    <message>
        <source>LFO Controller</source>
        <translation>Controller dell&apos;LFO</translation>
    </message>
    <message>
        <source>Base value</source>
        <translation>Valore di base</translation>
    </message>
    <message>
        <source>Oscillator speed</source>
        <translation>Velocità dell&apos;oscillatore</translation>
    </message>
    <message>
        <source>Oscillator amount</source>
        <translation>Quantità di oscillatore</translation>
    </message>
    <message>
        <source>Oscillator phase</source>
        <translation>Fase dell&apos;oscillatore</translation>
    </message>
    <message>
        <source>Oscillator waveform</source>
        <translation>Forma d&apos;onda dell&apos;oscillatore</translation>
    </message>
    <message>
        <source>Frequency Multiplier</source>
        <translation>Moltiplicatore della frequenza</translation>
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
        <translation>Controller dell&apos;LFO</translation>
    </message>
    <message>
        <source>BASE</source>
        <translation>BASE</translation>
    </message>
    <message>
        <source>Base amount:</source>
        <translation>Quantità di base:</translation>
    </message>
    <message>
        <source>todo</source>
        <translation>da fare</translation>
    </message>
    <message>
        <source>SPD</source>
        <translation>VEL</translation>
    </message>
    <message>
        <source>LFO-speed:</source>
        <translation>Velocità dell&apos;LFO:</translation>
    </message>
    <message>
        <source>Use this knob for setting speed of the LFO. The bigger this value the faster the LFO oscillates and the faster the effect.</source>
        <translation>Questa manopola imposta la velocità dell&apos;LFO selezionato. Più grande è questo valore più velocemente l&apos;LFO oscillerà e più veloce sarà l&apos;effetto.</translation>
    </message>
    <message>
        <source>AMT</source>
        <translation>Q.TÀ</translation>
    </message>
    <message>
        <source>Modulation amount:</source>
        <translation>Quantità di modulazione:</translation>
    </message>
    <message>
        <source>Use this knob for setting modulation amount of the LFO. The bigger this value, the more the connected control (e.g. volume or cutoff-frequency) will be influenced by the LFO.</source>
        <translation>Questa manopola imposta la quantità di modulazione dell&apos;LFO selezionato. Più grande è questo valore più la variabile selezionata (ad es. il volume o la frequenza di taglio) sarà influenzata da questo LFO.</translation>
    </message>
    <message>
        <source>PHS</source>
        <translation>FASE</translation>
    </message>
    <message>
        <source>Phase offset:</source>
        <translation>Scostamento della fase:</translation>
    </message>
    <message>
        <source>degrees</source>
        <translation>gradi</translation>
    </message>
    <message>
        <source>With this knob you can set the phase offset of the LFO. That means you can move the point within an oscillation where the oscillator begins to oscillate. For example if you have a sine-wave and have a phase-offset of 180 degrees the wave will first go down. It&apos;s the same with a square-wave.</source>
        <translation>Questa manopola regola lo scostamento della fase per l&apos;LFO. Ciò significa spostare il punto dell&apos;oscillazione da cui parte l&apos;oscillatore. Per esempio, con un&apos;onda sinusoidale e uno scostamento della fase di 180 gradi, l&apos;onda inizierà scendendo. Lo stesso vale per un&apos;onda quadra.</translation>
    </message>
    <message>
        <source>Click here for a sine-wave.</source>
        <translation>Cliccando qui si ha un&apos;onda sinusoidale.</translation>
    </message>
    <message>
        <source>Click here for a triangle-wave.</source>
        <translation>Cliccando qui si ottiene un&apos;onda triangolare.</translation>
    </message>
    <message>
        <source>Click here for a saw-wave.</source>
        <translation>Cliccando qui si ottiene un&apos;onda a dente di sega.</translation>
    </message>
    <message>
        <source>Click here for a square-wave.</source>
        <translation>Cliccando qui si ottiene un&apos;onda quadra.</translation>
    </message>
    <message>
        <source>Click here for a a moog saw-wave.</source>
        <translation>Cliccando qui si ha un&apos;onda a dente di sega moog.</translation>
    </message>
    <message>
        <source>Click here for an exponential wave.</source>
        <translation>Cliccando qui si ha un&apos;onda esponenziale.</translation>
    </message>
    <message>
        <source>Click here for white-noise.</source>
        <translation>Cliccando qui si ottiene rumore bianco.</translation>
    </message>
    <message>
        <source>Click here for a user-defined shape.
Double click to pick a file.</source>
        <translation>Cliccando qui si usa un&apos;onda definita dall&apos;utente.
Fare doppio click per scegliere il file dell&apos;onda.</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <source>Working directory</source>
        <translation>Directory di lavoro</translation>
    </message>
    <message>
        <source>The LMMS working directory %1 does not exist. Create it now? You can change the directory later via Edit -&gt; Settings.</source>
        <translation>La directory di lavoro di LMMS %1 non esiste. La creo adesso? Questa directory può essere cambiata in un secondo momento dal menu Modifica -&gt; Impostazioni.</translation>
    </message>
    <message>
        <source>Could not save config-file</source>
        <translation>Non è stato possibile salvare il file di configurazione</translation>
    </message>
    <message>
        <source>Could not save configuration file %1. You&apos;re probably not permitted to write to this file.
Please make sure you have write-access to the file and try again.</source>
        <translation>Non è stato possibile salvare il file di configurazione %1. Probabilmente non hai i permessi di scrittura per questo file.
Assicurati di avere i permessi in scrittura per il file e riprova.</translation>
    </message>
    <message>
        <source>&amp;Project</source>
        <translation>&amp;Progetto</translation>
    </message>
    <message>
        <source>&amp;New</source>
        <translation>&amp;Nuovo</translation>
    </message>
    <message>
        <source>&amp;Open...</source>
        <translation>&amp;Apri...</translation>
    </message>
    <message>
        <source>Recently opened projects</source>
        <translation>Progetti aperti di recente</translation>
    </message>
    <message>
        <source>&amp;Save</source>
        <translation>&amp;Salva</translation>
    </message>
    <message>
        <source>Save &amp;As...</source>
        <translation>Salva &amp;Con Nome...</translation>
    </message>
    <message>
        <source>Import...</source>
        <translation>Importa...</translation>
    </message>
    <message>
        <source>E&amp;xport...</source>
        <translation>E&amp;sporta...</translation>
    </message>
    <message>
        <source>&amp;Quit</source>
        <translation>&amp;Esci</translation>
    </message>
    <message>
        <source>&amp;Edit</source>
        <translation>&amp;Modifica</translation>
    </message>
    <message>
        <source>Settings</source>
        <translation>Impostazioni</translation>
    </message>
    <message>
        <source>&amp;Tools</source>
        <translation>S&amp;trumenti</translation>
    </message>
    <message>
        <source>&amp;Help</source>
        <translation>&amp;Aiuto</translation>
    </message>
    <message>
        <source>Online help</source>
        <translation>Aiuto in linea</translation>
    </message>
    <message>
        <source>Help</source>
        <translation>Aiuto</translation>
    </message>
    <message>
        <source>What&apos;s this?</source>
        <translation>Cos&apos;è questo?</translation>
    </message>
    <message>
        <source>About</source>
        <translation>Informazioni su</translation>
    </message>
    <message>
        <source>Create new project</source>
        <translation>Crea un nuovo progetto</translation>
    </message>
    <message>
        <source>Create new project from template</source>
        <translation>Crea un nuovo progetto da un modello</translation>
    </message>
    <message>
        <source>Open existing project</source>
        <translation>Apri un progetto esistente</translation>
    </message>
    <message>
        <source>Recently opened project</source>
        <translation>Progetti aperti di recente</translation>
    </message>
    <message>
        <source>Save current project</source>
        <translation>Salva questo progetto</translation>
    </message>
    <message>
        <source>Export current project</source>
        <translation>Esporta questo progetto</translation>
    </message>
    <message>
        <source>Show/hide Song-Editor</source>
        <translation>Mostra/nascondi il Song-Editor</translation>
    </message>
    <message>
        <source>By pressing this button, you can show or hide the Song-Editor. With the help of the Song-Editor you can edit song-playlist and specify when which track should be played. You can also insert and move samples (e.g. rap samples) directly into the playlist.</source>
        <translation>Questo pulsante mostra o nasconde il Song-Editor. Con l&apos;aiuto del Song-Editor è possibile modificare la playlist e specificare quando ogni traccia viene riprodotta. Si possono anche inserire e spostare i campioni (ad es. campioni rap) direttamente nella lista di riproduzione.</translation>
    </message>
    <message>
        <source>Show/hide Beat+Bassline Editor</source>
        <translation>Mostra/nascondi il Beat+Bassline Editor</translation>
    </message>
    <message>
        <source>By pressing this button, you can show or hide the Beat+Bassline Editor. The Beat+Bassline Editor is needed for creating beats, and for opening, adding, and removing channels, and for cutting, copying and pasting beat and bassline-patterns, and for other things like that.</source>
        <translation>Questo pulsante mostra o nasconde il Beat+Bassline Editor. Il Beat+Bassline Editor serve per creare beats, aprire, aggiungere e togliere canali, tagliare, copiare e incollare pattern beat e pattern bassline.</translation>
    </message>
    <message>
        <source>Show/hide Piano-Roll</source>
        <translation>Mostra/nascondi il Piano-Roll</translation>
    </message>
    <message>
        <source>Click here to show or hide the Piano-Roll. With the help of the Piano-Roll you can edit melodies in an easy way.</source>
        <translation>Questo pulsante mostra o nasconde il Piano-Roll. Con l&apos;aiuto del Piano-Roll è possibile modificare sequenze melodiche in modo semplice.</translation>
    </message>
    <message>
        <source>Show/hide Automation Editor</source>
        <translation>Mostra/nascondi l&apos;Editor dell&apos;automazione</translation>
    </message>
    <message>
        <source>Click here to show or hide the Automation Editor. With the help of the Automation Editor you can edit dynamic values in an easy way.</source>
        <translation>Questo pulsante mostra o nasconde l&apos;editor dell&apos;automazione. Con l&apos;aiuto dell&apos;editor dell&apos;automazione è possibile rendere dinamici alcuni valori in modo semplice.</translation>
    </message>
    <message>
        <source>Show/hide FX Mixer</source>
        <translation>Mostra/nascondi il mixer degli effetti</translation>
    </message>
    <message>
        <source>Click here to show or hide the FX Mixer. The FX Mixer is a very powerful tool for managing effects for your song. You can insert effects into different effect-channels.</source>
        <translation>Questo pulsante mostra o nasconde il mixer degli effetti. Il mixer degli effetti è uno strumento molto potente per gestire gli effetti della canzone. È possibile inserire effetti in più canali.</translation>
    </message>
    <message>
        <source>Show/hide project notes</source>
        <translation>Mostra/nascondi le note del progetto</translation>
    </message>
    <message>
        <source>Click here to show or hide the project notes window. In this window you can put down your project notes.</source>
        <translation>Questo pulsante mostra o nasconde la finestra delle note del progetto. Qui è possibile scrivere le note per il progetto.</translation>
    </message>
    <message>
        <source>Show/hide controller rack</source>
        <translation>Mostra/nascondi il rack di controller</translation>
    </message>
    <message>
        <source>Untitled</source>
        <translation>Senza_nome</translation>
    </message>
    <message>
        <source>LMMS %1</source>
        <translation>LMMS %1</translation>
    </message>
    <message>
        <source>Project not saved</source>
        <translation>Progetto non salvato</translation>
    </message>
    <message>
        <source>The current project was modified since last saving. Do you want to save it now?</source>
        <translation>Questo progetto è stato modificato dopo l&apos;ultimo salvataggio. Vuoi salvarlo adesso?</translation>
    </message>
    <message>
        <source>Open project</source>
        <translation>Apri progetto</translation>
    </message>
    <message>
        <source>Save project</source>
        <translation>Salva progetto</translation>
    </message>
    <message>
        <source>Help not available</source>
        <translation>Aiuto non disponibile</translation>
    </message>
    <message>
        <source>Currently there&apos;s no help available in LMMS.
Please visit http://lmms.sf.net/wiki for documentation on LMMS.</source>
        <translation>Al momento non è disponibile alcun aiuto in LMMS.
Visitare http://lmms.sf.net/wiki  per la documentazione di LMMS.</translation>
    </message>
    <message>
        <source>My projects</source>
        <translation>I miei progetti</translation>
    </message>
    <message>
        <source>My samples</source>
        <translation>I miei campioni</translation>
    </message>
    <message>
        <source>My presets</source>
        <translation>I miei preset</translation>
    </message>
    <message>
        <source>My home</source>
        <translation>Cartelle utente</translation>
    </message>
    <message>
        <source>My computer</source>
        <translation>Cartelle computer</translation>
    </message>
    <message>
        <source>Root directory</source>
        <translation>Directory principale</translation>
    </message>
    <message>
        <source>Save as new &amp;version</source>
        <translation>Salva come nuova &amp;versione</translation>
    </message>
    <message>
        <source>E&amp;xport tracks...</source>
        <translation>E&amp;sporta traccie...</translation>
    </message>
    <message>
        <source>LMMS (*.mmp *.mmpz)</source>
        <translation></translation>
    </message>
    <message>
        <source>LMMS Project (*.mmp *.mmpz);;LMMS Project Template (*.mpt)</source>
        <translation>Progetto LMMS (*mmp *mmpz);;Template di Progetto LMMS (*.mpt)</translation>
    </message>
    <message>
        <source>Version %1</source>
        <translation>Versione %1</translation>
    </message>
    <message>
        <source>Project recovery</source>
        <translation>Recupero del progetto</translation>
    </message>
    <message>
        <source>It looks like the last session did not end properly. Do you want to recover the project of this session?</source>
        <translation>Sembra che l&apos;ultima sessione non sia stata chiusa correttamente. Vuoi recuperare il progetto di quella sessione?</translation>
    </message>
    <message>
        <source>Configuration file</source>
        <translation>File di configurazione</translation>
    </message>
    <message>
        <source>Error while parsing configuration file at line %1:%2: %3</source>
        <translation>Si è riscontrato un errore nell&apos;analisi del file di configurazione alle linee %1:%2: %3</translation>
    </message>
</context>
<context>
    <name>MeterDialog</name>
    <message>
        <source>Meter Numerator</source>
        <translation>Numeratore della misura</translation>
    </message>
    <message>
        <source>Meter Denominator</source>
        <translation>Denominatore della misura</translation>
    </message>
    <message>
        <source>TIME SIG</source>
        <translation>TEMPO</translation>
    </message>
</context>
<context>
    <name>MeterModel</name>
    <message>
        <source>Numerator</source>
        <translation>Numeratore</translation>
    </message>
    <message>
        <source>Denominator</source>
        <translation>Denominatore</translation>
    </message>
</context>
<context>
    <name>MidiAlsaRaw::setupWidget</name>
    <message>
        <source>DEVICE</source>
        <translation>PERIFERICA</translation>
    </message>
</context>
<context>
    <name>MidiAlsaSeq::setupWidget</name>
    <message>
        <source>DEVICE</source>
        <translation>PERIFERICA</translation>
    </message>
</context>
<context>
    <name>MidiController</name>
    <message>
        <source>MIDI Controller</source>
        <translation>Controller MIDI</translation>
    </message>
    <message>
        <source>unnamed_midi_controller</source>
        <translation>controller_midi_senza_nome</translation>
    </message>
</context>
<context>
    <name>MidiImport</name>
    <message>
        <source>Setup incomplete</source>
        <translation>Impostazioni incomplete</translation>
    </message>
    <message>
        <source>You do not have set up a default soundfont in the settings dialog (Edit-&gt;Settings). Therefore no sound will be played back after importing this MIDI file. You should download a General MIDI soundfont, specify it in settings dialog and try again.</source>
        <translation>Non hai impostato un soundfont di base (Modifica-&gt;Impostazioni). Quindi non sarà riprodotto alcun suono dopo aver importato questo file MIDI. Prova a scaricare un soundfont MIDI generico, specifica la sua posizione nelle impostazioni e prova di nuovo.</translation>
    </message>
    <message>
        <source>You did not compile LMMS with support for SoundFont2 player, which is used to add default sound to imported MIDI files. Therefore no sound will be played back after importing this MIDI file.</source>
        <translation>Non hai compilato LMMS con il supporto per SoundFont2 Player, che viene usato per aggiungere suoni predefiniti ai file MIDI importati. Quindi, nessun suono verrà riprodotto dopo aver aperto questo file MIDI.</translation>
    </message>
</context>
<context>
    <name>MidiOss::setupWidget</name>
    <message>
        <source>DEVICE</source>
        <translation>PERIFERICA</translation>
    </message>
</context>
<context>
    <name>MidiPort</name>
    <message>
        <source>Input channel</source>
        <translation>Canale di ingresso</translation>
    </message>
    <message>
        <source>Output channel</source>
        <translation>Canale di uscita</translation>
    </message>
    <message>
        <source>Input controller</source>
        <translation>Controller in entrata</translation>
    </message>
    <message>
        <source>Output controller</source>
        <translation>Controller in uscita</translation>
    </message>
    <message>
        <source>Fixed input velocity</source>
        <translation>Velocity fissa in ingresso</translation>
    </message>
    <message>
        <source>Fixed output velocity</source>
        <translation>Velocity fissa in uscita</translation>
    </message>
    <message>
        <source>Output MIDI program</source>
        <translation>Programma MIDI in uscita</translation>
    </message>
    <message>
        <source>Receive MIDI-events</source>
        <translation>Ricevi eventi MIDI</translation>
    </message>
    <message>
        <source>Send MIDI-events</source>
        <translation>Invia eventi MIDI</translation>
    </message>
    <message>
        <source>Fixed output note</source>
        <translation>Nota fissa in uscita</translation>
    </message>
</context>
<context>
    <name>OscillatorObject</name>
    <message>
        <source>Osc %1 volume</source>
        <translation>Volume osc %1</translation>
    </message>
    <message>
        <source>Osc %1 panning</source>
        <translation>Panning osc %1</translation>
    </message>
    <message>
        <source>Osc %1 coarse detuning</source>
        <translation>Intonazione osc %1</translation>
    </message>
    <message>
        <source>Osc %1 fine detuning left</source>
        <translation>Intonazione precisa osc %1 sinistra</translation>
    </message>
    <message>
        <source>Osc %1 fine detuning right</source>
        <translation>Intonazione precisa osc %1 destra</translation>
    </message>
    <message>
        <source>Osc %1 phase-offset</source>
        <translation>Scostamento fase osc %1</translation>
    </message>
    <message>
        <source>Osc %1 stereo phase-detuning</source>
        <translation>Intonazione fase stereo osc %1</translation>
    </message>
    <message>
        <source>Osc %1 wave shape</source>
        <translation>Forma d&apos;onda Osc %1</translation>
    </message>
    <message>
        <source>Modulation type %1</source>
        <translation>Modulazione di tipo %1</translation>
    </message>
    <message>
        <source>Osc %1 waveform</source>
        <translation>Forma d&apos;onda osc %1</translation>
    </message>
</context>
<context>
    <name>PatmanView</name>
    <message>
        <source>Open other patch</source>
        <translation>Apri un&apos;altra patch</translation>
    </message>
    <message>
        <source>Click here to open another patch-file. Loop and Tune settings are not reset.</source>
        <translation>Clicca qui per aprire un altro file di patch. Le impostazioni di ripetizione e intonazione non vengono reimpostate.</translation>
    </message>
    <message>
        <source>Loop</source>
        <translation>Ripetizione</translation>
    </message>
    <message>
        <source>Loop mode</source>
        <translation>Modalità ripetizione</translation>
    </message>
    <message>
        <source>Here you can toggle the Loop mode. If enabled, PatMan will use the loop information available in the file.</source>
        <translation>Qui puoi scegliere la modalità di ripetizione. Se abilitata, PatMan userà l&apos;informazione sulla ripetizione disponibile nel file.</translation>
    </message>
    <message>
        <source>Tune</source>
        <translation>Intonazione</translation>
    </message>
    <message>
        <source>Tune mode</source>
        <translation>Modalità intonazione</translation>
    </message>
    <message>
        <source>Here you can toggle the Tune mode. If enabled, PatMan will tune the sample to match the note&apos;s frequency.</source>
        <translation>Qui puoi scegliere la modalità di intonazione. Se abilitata, PatMan intonerà il campione alla frequenza della nota.</translation>
    </message>
    <message>
        <source>No file selected</source>
        <translation>Nessun file selezionato</translation>
    </message>
    <message>
        <source>Open patch file</source>
        <translation>Apri file di patch</translation>
    </message>
    <message>
        <source>Patch-Files (*.pat)</source>
        <translation>File di patch (*.pat)</translation>
    </message>
</context>
<context>
    <name>PeakController</name>
    <message>
        <source>Peak Controller</source>
        <translation>Controller dei picchi</translation>
    </message>
    <message>
        <source>Peak Controller Bug</source>
        <translation>Bug del controller dei picchi</translation>
    </message>
    <message>
        <source>Due to a bug in older version of LMMS, the peak controllers may not be connect properly. Please ensure that peak controllers are connected properly and re-save this file. Sorry for any inconvenience caused.</source>
        <translation>A causa di un bug nelle versioni precedenti di LMMS, i controller dei picchi potrebbero non essere connessi come dovuto. Assicurati che i controller dei picchi siano connessi e salva questo file di nuovo. Ci scusiamo per gli inconvenienti causati.</translation>
    </message>
</context>
<context>
    <name>PeakControllerDialog</name>
    <message>
        <source>PEAK</source>
        <translation>PICCO</translation>
    </message>
    <message>
        <source>LFO Controller</source>
        <translation>Controller dell&apos;LFO</translation>
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
        <translation>Quantità di base:</translation>
    </message>
    <message>
        <source>Modulation amount:</source>
        <translation>Quantità di modulazione:</translation>
    </message>
    <message>
        <source>Attack:</source>
        <translation>Attacco:</translation>
    </message>
    <message>
        <source>Release:</source>
        <translation>Rilascio:</translation>
    </message>
    <message>
        <source>AMNT</source>
        <translation>Q.TÀ</translation>
    </message>
    <message>
        <source>MULT</source>
        <translation>MOLT</translation>
    </message>
    <message>
        <source>Amount Multiplicator:</source>
        <translation>Moltiplicatore di quantità:</translation>
    </message>
    <message>
        <source>ATCK</source>
        <translation>ATCC</translation>
    </message>
    <message>
        <source>DCAY</source>
        <translation>DCAD</translation>
    </message>
</context>
<context>
    <name>PeakControllerEffectControls</name>
    <message>
        <source>Base value</source>
        <translation>Valore di base</translation>
    </message>
    <message>
        <source>Modulation amount</source>
        <translation>Quantità di modulazione</translation>
    </message>
    <message>
        <source>Mute output</source>
        <translation>Silenzia l&apos;output</translation>
    </message>
    <message>
        <source>Attack</source>
        <translation>Attacco</translation>
    </message>
    <message>
        <source>Release</source>
        <translation>Rilascio</translation>
    </message>
    <message>
        <source>Abs Value</source>
        <translation>Valore Assoluto</translation>
    </message>
    <message>
        <source>Amount Multiplicator</source>
        <translation>Moltiplicatore della quantità</translation>
    </message>
</context>
<context>
    <name>PianoView</name>
    <message>
        <source>Base note</source>
        <translation>Nota base</translation>
    </message>
</context>
<context>
    <name>Plugin</name>
    <message>
        <source>Plugin not found</source>
        <translation>Plugin non trovato</translation>
    </message>
    <message>
        <source>The plugin &quot;%1&quot; wasn&apos;t found or could not be loaded!
Reason: &quot;%2&quot;</source>
        <translation>Il plugin &quot;%1&quot; non è stato trovato o non è stato possibile caricarlo!
Motivo: &quot;%2&quot;</translation>
    </message>
    <message>
        <source>Error while loading plugin</source>
        <translation>Errore nel caricamento del plugin</translation>
    </message>
    <message>
        <source>Failed to load plugin &quot;%1&quot;!</source>
        <translation>Non è stato possibile caricare il plugin &quot;%1&quot;!</translation>
    </message>
</context>
<context>
    <name>ProjectRenderer</name>
    <message>
        <source>WAV-File (*.wav)</source>
        <translation>File WAV (*.wav)</translation>
    </message>
    <message>
        <source>Compressed OGG-File (*.ogg)</source>
        <translation>File in formato OGG compresso (*.ogg)</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <source>C</source>
        <comment>Note name</comment>
        <translation>Do</translation>
    </message>
    <message>
        <source>Db</source>
        <comment>Note name</comment>
        <translation>Dob</translation>
    </message>
    <message>
        <source>C#</source>
        <comment>Note name</comment>
        <translation>Do#</translation>
    </message>
    <message>
        <source>D</source>
        <comment>Note name</comment>
        <translation>Re</translation>
    </message>
    <message>
        <source>Eb</source>
        <comment>Note name</comment>
        <translation>Mib</translation>
    </message>
    <message>
        <source>D#</source>
        <comment>Note name</comment>
        <translation>Re#</translation>
    </message>
    <message>
        <source>E</source>
        <comment>Note name</comment>
        <translation>Mi</translation>
    </message>
    <message>
        <source>Fb</source>
        <comment>Note name</comment>
        <translation>Fab</translation>
    </message>
    <message>
        <source>Gb</source>
        <comment>Note name</comment>
        <translation>Solb</translation>
    </message>
    <message>
        <source>F#</source>
        <comment>Note name</comment>
        <translation>Fa#</translation>
    </message>
    <message>
        <source>G</source>
        <comment>Note name</comment>
        <translation>Sol</translation>
    </message>
    <message>
        <source>Ab</source>
        <comment>Note name</comment>
        <translation>Lab</translation>
    </message>
    <message>
        <source>G#</source>
        <comment>Note name</comment>
        <translation>Sol#</translation>
    </message>
    <message>
        <source>A</source>
        <comment>Note name</comment>
        <translation>La</translation>
    </message>
    <message>
        <source>Bb</source>
        <comment>Note name</comment>
        <translation>Sib</translation>
    </message>
    <message>
        <source>A#</source>
        <comment>Note name</comment>
        <translation>La#</translation>
    </message>
    <message>
        <source>B</source>
        <comment>Note name</comment>
        <translation>Si</translation>
    </message>
</context>
<context>
    <name>QWidget</name>
    <message>
        <source>Name: </source>
        <translation>Nome:</translation>
    </message>
    <message>
        <source>Maker: </source>
        <translation>Autore:</translation>
    </message>
    <message>
        <source>Requires Real Time: </source>
        <translation>Richiede Real Time:</translation>
    </message>
    <message>
        <source>Yes</source>
        <translation>Sì</translation>
    </message>
    <message>
        <source>No</source>
        <translation>No</translation>
    </message>
    <message>
        <source>Real Time Capable: </source>
        <translation>Abilitato al Real Time:</translation>
    </message>
    <message>
        <source>Channels In: </source>
        <translation>Canali in ingresso:</translation>
    </message>
    <message>
        <source>Channels Out: </source>
        <translation>Canali in uscita:</translation>
    </message>
    <message>
        <source>File: </source>
        <translation>File:</translation>
    </message>
    <message>
        <source>Copyright: </source>
        <translation>Copyright:</translation>
    </message>
    <message>
        <source>In Place Broken: </source>
        <translation>In Place Broken:</translation>
    </message>
</context>
<context>
    <name>SampleBuffer</name>
    <message>
        <source>Open audio file</source>
        <translation>Apri file audio</translation>
    </message>
    <message>
        <source>All Audio-Files (*.wav *.ogg *.ds *.flac *.spx *.voc *.aif *.aiff *.au *.raw *.mp3)</source>
        <translation>Tutti i file audio (*.wav *.ogg *.ds *.flac *.spx *.voc *.aif *.aiff *.au *.raw *.mp3)</translation>
    </message>
    <message>
        <source>Wave-Files (*.wav)</source>
        <translation>File wave (*.wav)</translation>
    </message>
    <message>
        <source>OGG-Files (*.ogg)</source>
        <translation>File OGG (*.ogg)</translation>
    </message>
    <message>
        <source>DrumSynth-Files (*.ds)</source>
        <translation>File DrumSynth (*.ds)</translation>
    </message>
    <message>
        <source>FLAC-Files (*.flac)</source>
        <translation>File FLAC (*.flac)</translation>
    </message>
    <message>
        <source>SPEEX-Files (*.spx)</source>
        <translation>File SPEEX (*.spx)</translation>
    </message>
    <message>
        <source>MP3-Files (*.mp3)</source>
        <translation>File MP3 (*.mp3)</translation>
    </message>
    <message>
        <source>VOC-Files (*.voc)</source>
        <translation>File VOC (*.voc)</translation>
    </message>
    <message>
        <source>AIFF-Files (*.aif *.aiff)</source>
        <translation>File AIFF (*.aif *.aiff)</translation>
    </message>
    <message>
        <source>AU-Files (*.au)</source>
        <translation>File AU (*.au)</translation>
    </message>
    <message>
        <source>RAW-Files (*.raw)</source>
        <translation>File RAW (*.raw)</translation>
    </message>
</context>
<context>
    <name>SampleTCOView</name>
    <message>
        <source>double-click to select sample</source>
        <translation>Fare doppio click per selezionare il campione</translation>
    </message>
    <message>
        <source>Delete (middle mousebutton)</source>
        <translation>Elimina (tasto centrale del mouse)</translation>
    </message>
    <message>
        <source>Cut</source>
        <translation>Taglia</translation>
    </message>
    <message>
        <source>Copy</source>
        <translation>Copia</translation>
    </message>
    <message>
        <source>Paste</source>
        <translation>Incolla</translation>
    </message>
    <message>
        <source>Mute/unmute (&lt;Ctrl&gt; + middle click)</source>
        <translation>Attiva/disattiva la modalità muta (&lt;Ctrl&gt; + tasto centrale)</translation>
    </message>
    <message>
        <source>Set/clear record</source>
        <translation>Set/clear record</translation>
    </message>
</context>
<context>
    <name>SampleTrack</name>
    <message>
        <source>Sample track</source>
        <translation>Traccia di campione</translation>
    </message>
    <message>
        <source>Volume</source>
        <translation>Volume</translation>
    </message>
</context>
<context>
    <name>SampleTrackView</name>
    <message>
        <source>Track volume</source>
        <translation>Volume della traccia</translation>
    </message>
    <message>
        <source>Channel volume:</source>
        <translation>Volume del canale:</translation>
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
        <translation>Sync del tempo</translation>
    </message>
    <message>
        <source>No Sync</source>
        <translation>Non in Sync</translation>
    </message>
    <message>
        <source>Eight beats</source>
        <translation>Otto battiti</translation>
    </message>
    <message>
        <source>Whole note</source>
        <translation>Un intero</translation>
    </message>
    <message>
        <source>Half note</source>
        <translation>Una metà</translation>
    </message>
    <message>
        <source>Quarter note</source>
        <translation>Quarto</translation>
    </message>
    <message>
        <source>8th note</source>
        <translation>Ottavo</translation>
    </message>
    <message>
        <source>16th note</source>
        <translation>Sedicesimo</translation>
    </message>
    <message>
        <source>32nd note</source>
        <translation>Trentaduesimo</translation>
    </message>
    <message>
        <source>Custom...</source>
        <translation>Personalizzato...</translation>
    </message>
    <message>
        <source>&amp;Help</source>
        <translation>&amp;Aiuto</translation>
    </message>
    <message>
        <source>Custom </source>
        <translation>Personalizzato</translation>
    </message>
    <message>
        <source>Synced to Eight Beats</source>
        <translation>In sync con otto battiti</translation>
    </message>
    <message>
        <source>Synced to Whole Note</source>
        <translation>In sync con un intero</translation>
    </message>
    <message>
        <source>Synced to Half Note</source>
        <translation>In sync con un mezzo</translation>
    </message>
    <message>
        <source>Synced to Quarter Note</source>
        <translation>In sync con quarti</translation>
    </message>
    <message>
        <source>Synced to 8th Note</source>
        <translation>In sync con ottavi</translation>
    </message>
    <message>
        <source>Synced to 16th Note</source>
        <translation>In sync con 16simi</translation>
    </message>
    <message>
        <source>Synced to 32nd Note</source>
        <translation>In sync con 32simi</translation>
    </message>
</context>
<context>
    <name>TimeDisplayWidget</name>
    <message>
        <source>click to change time units</source>
        <translation>Clicca per cambiare l&apos;unità di tempo visualizzata</translation>
    </message>
</context>
<context>
    <name>TrackContainer</name>
    <message>
        <source>Couldn&apos;t import file</source>
        <translation>Non è stato possibile importare il file</translation>
    </message>
    <message>
        <source>Couldn&apos;t find a filter for importing file %1.
You should convert this file into a format supported by LMMS using another software.</source>
        <translation>Non è stato possibile trovare un filtro per importare il file %1.
È necessario convertire questo file in un formato supportato da LMMS usando un altro programma.</translation>
    </message>
    <message>
        <source>Couldn&apos;t open file</source>
        <translation>Non è stato possibile aprire il file</translation>
    </message>
    <message>
        <source>Couldn&apos;t open file %1 for reading.
Please make sure you have read-permission to the file and the directory containing the file and try again!</source>
        <translation>Non è stato possibile aprire il file %1 in lettura.
Assicurarsi di avere i permessi in lettura per il file e per la directory che lo contiene e riprovare!</translation>
    </message>
    <message>
        <source>Loading project...</source>
        <translation>Caricamento del progetto...</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation>Annulla</translation>
    </message>
    <message>
        <source>Please wait...</source>
        <translation>Attendere...</translation>
    </message>
    <message>
        <source>Importing MIDI-file...</source>
        <translation>Importazione del file MIDI...</translation>
    </message>
    <message>
        <source>Importing FLP-file...</source>
        <translation>Importazione del file FLP...</translation>
    </message>
</context>
<context>
    <name>TripleOscillatorView</name>
    <message>
        <source>Use phase modulation for modulating oscillator 2 with oscillator 1</source>
        <translation>Usare la modulazione di fase per modulare l&apos;oscillatore 2 con l&apos;oscillatore 1</translation>
    </message>
    <message>
        <source>Use amplitude modulation for modulating oscillator 2 with oscillator 1</source>
        <translation>Usare la modulazione di amplificazione per modulare l&apos;oscillatore 2 con l&apos;oscillatore 1</translation>
    </message>
    <message>
        <source>Mix output of oscillator 1 &amp; 2</source>
        <translation>Miscelare gli oscillatori 1 e 2</translation>
    </message>
    <message>
        <source>Synchronize oscillator 1 with oscillator 2</source>
        <translation>Sincronizzare l&apos;oscillatore 1 con l&apos;oscillatore 2</translation>
    </message>
    <message>
        <source>Use frequency modulation for modulating oscillator 2 with oscillator 1</source>
        <translation>Usare la modulazione di frequenza per modulare l&apos;oscillatore 2 con l&apos;oscillatore 1</translation>
    </message>
    <message>
        <source>Use phase modulation for modulating oscillator 3 with oscillator 2</source>
        <translation>Usare la modulazione di fase per modulare l&apos;oscillatore 3 con l&apos;oscillatore 2</translation>
    </message>
    <message>
        <source>Use amplitude modulation for modulating oscillator 3 with oscillator 2</source>
        <translation>Usare la modulazione di amplificazione per modulare l&apos;oscillatore 3 con l&apos;oscillatore 2</translation>
    </message>
    <message>
        <source>Mix output of oscillator 2 &amp; 3</source>
        <translation>Miscelare gli oscillatori 2 e 3</translation>
    </message>
    <message>
        <source>Synchronize oscillator 2 with oscillator 3</source>
        <translation>Sincronizzare l&apos;oscillatore 2 con l&apos;oscillatore 3</translation>
    </message>
    <message>
        <source>Use frequency modulation for modulating oscillator 3 with oscillator 2</source>
        <translation>Usare la modulazione di frequenza per modulare l&apos;oscillatore 3 con l&apos;oscillatore 2</translation>
    </message>
    <message>
        <source>Osc %1 volume:</source>
        <translation>Volume osc %1:</translation>
    </message>
    <message>
        <source>With this knob you can set the volume of oscillator %1. When setting a value of 0 the oscillator is turned off. Otherwise you can hear the oscillator as loud as you set it here.</source>
        <translation>Questa manopola regola il volume dell&apos;oscillatore %1. Un valore pari a 0 equivale a un oscillatore spento, gli altri valori impostano il volume corrispondente.</translation>
    </message>
    <message>
        <source>Osc %1 panning:</source>
        <translation>Panning osc %1:</translation>
    </message>
    <message>
        <source>With this knob you can set the panning of the oscillator %1. A value of -100 means 100% left and a value of 100 moves oscillator-output right.</source>
        <translation>Questa manopola regola il posizionamento nello spettro stereo dell&apos;oscillatore %1. Un valore pari a -100 significa tutto a sinistra mentre un valore pari a 100 significa tutto a destra.</translation>
    </message>
    <message>
        <source>Osc %1 coarse detuning:</source>
        <translation>Intonazione dell&apos;osc %1:</translation>
    </message>
    <message>
        <source>semitones</source>
        <translation>semitoni</translation>
    </message>
    <message>
        <source>With this knob you can set the coarse detuning of oscillator %1. You can detune the oscillator 12 semitones (1 octave) up and down. This is useful for creating sounds with a chord.</source>
        <translation>Questa manopola regola l&apos;intonazione, con la precisione di 1 semitono, dell&apos;oscillatore %1. L&apos;intonazione può essere variata di 24 semitoni (due ottave) in positivo e in negativo. Può essere usata per creare suoni con un accordo.</translation>
    </message>
    <message>
        <source>Osc %1 fine detuning left:</source>
        <translation>Intonazione precisa osc %1 sinistra:</translation>
    </message>
    <message>
        <source>cents</source>
        <translation>centesimi</translation>
    </message>
    <message>
        <source>With this knob you can set the fine detuning of oscillator %1 for the left channel. The fine-detuning is ranged between -100 cents and +100 cents. This is useful for creating &quot;fat&quot; sounds.</source>
        <translation>Questa manopola regola l&apos;intonazione precisa dell&apos;oscillatore %1 per il canale sinistro. La gamma per l&apos;intonazione di precisione va da -100 a +100 centesimi. Può essere usata per creare suoni &quot;grossi&quot;.</translation>
    </message>
    <message>
        <source>Osc %1 fine detuning right:</source>
        <translation>Intonazione precisa dell&apos;osc %1 - destra:</translation>
    </message>
    <message>
        <source>With this knob you can set the fine detuning of oscillator %1 for the right channel. The fine-detuning is ranged between -100 cents and +100 cents. This is useful for creating &quot;fat&quot; sounds.</source>
        <translation>Questa manopola regola l&apos;intonazione precisa dell&apos;oscillatore %1 per il canale destro. La gamma per l&apos;intonazione di precisione va da -100 a +100 centesimi. Può essere usata per creare suoni &quot;grossi&quot;.</translation>
    </message>
    <message>
        <source>Osc %1 phase-offset:</source>
        <translation>Scostamento fase dell&apos;osc %1:</translation>
    </message>
    <message>
        <source>degrees</source>
        <translation>gradi</translation>
    </message>
    <message>
        <source>With this knob you can set the phase-offset of oscillator %1. That means you can move the point within an oscillation where the oscillator begins to oscillate. For example if you have a sine-wave and have a phase-offset of 180 degrees the wave will first go down. It&apos;s the same with a square-wave.</source>
        <translation>Questa manopola regola lo scostamento della fase dell&apos;oscillatore %1. Ciò significa che è possibile spostare il punto in cui inizia l&apos;oscillazione. Per esempio, un&apos;onda sinusoidale e uno scostamento della fase di 180 gradi, fanno iniziare l&apos;onda scendendo. Lo stesso vale per un&apos;onda quadra.</translation>
    </message>
    <message>
        <source>Osc %1 stereo phase-detuning:</source>
        <translation>Intonazione fase stereo dell&apos;osc %1:</translation>
    </message>
    <message>
        <source>With this knob you can set the stereo phase-detuning of oscillator %1. The stereo phase-detuning specifies the size of the difference between the phase-offset of left and right channel. This is very good for creating wide stereo sounds.</source>
        <translation>Questa manopola regola l&apos;intonazione stereo della fase dell&apos;oscillatore %1. L&apos;intonazione stereo della fase specifica la differenza tra lo scostamento della fase del canale sinistro e quello destro. Questo è molto utile per creare suoni con grande ampiezza stereo.</translation>
    </message>
    <message>
        <source>Use a sine-wave for current oscillator.</source>
        <translation>Utilizzare un&apos;onda sinusoidale per questo oscillatore.</translation>
    </message>
    <message>
        <source>Use a triangle-wave for current oscillator.</source>
        <translation>Utilizzare un&apos;onda triangolare per questo oscillatore.</translation>
    </message>
    <message>
        <source>Use a saw-wave for current oscillator.</source>
        <translation>Utilizzare un&apos;onda a dente di sega per questo oscillatore.</translation>
    </message>
    <message>
        <source>Use a square-wave for current oscillator.</source>
        <translation>Utilizzare un&apos;onda quadra per questo oscillatore.</translation>
    </message>
    <message>
        <source>Use a moog-like saw-wave for current oscillator.</source>
        <translation>Utilizzare un&apos;onda di tipo moog per questo oscillatore.</translation>
    </message>
    <message>
        <source>Use an exponential wave for current oscillator.</source>
        <translation>Utilizzare un&apos;onda esponenziale per questo oscillatore.</translation>
    </message>
    <message>
        <source>Use white-noise for current oscillator.</source>
        <translation>Utilizzare rumore bianco per questo oscillatore.</translation>
    </message>
    <message>
        <source>Use a user-defined waveform for current oscillator.</source>
        <translation>Utilizzare un&apos;onda personalizzata per questo oscillatore.</translation>
    </message>
</context>
<context>
    <name>Ui</name>
    <message>
        <source>Contributors ordered by number of commits:</source>
        <translation>Hanno collaborato: (ordinati per numero di contributi)</translation>
    </message>
    <message>
        <source>Involved</source>
        <translation>Coinvolti</translation>
    </message>
</context>
<context>
    <name>VersionedSaveDialog</name>
    <message>
        <source>Increment version number</source>
        <translation>Nuova versione</translation>
    </message>
    <message>
        <source>Decrement version number</source>
        <translation>Riduci numero versione</translation>
    </message>
</context>
<context>
    <name>VestigeInstrumentView</name>
    <message>
        <source>Open other VST-plugin</source>
        <translation>Apri un altro plugin VST</translation>
    </message>
    <message>
        <source>Click here, if you want to open another VST-plugin. After clicking on this button, a file-open-dialog appears and you can select your file.</source>
        <translation>Clicca qui per aprire un altro plugin VST. Una volta cliccato questo pulsante, si aprirà una finestra di dialogo dove potrai selezionare il file.</translation>
    </message>
    <message>
        <source>Show/hide GUI</source>
        <translation>Mostra/nascondi l&apos;interfaccia</translation>
    </message>
    <message>
        <source>Click here to show or hide the graphical user interface (GUI) of your VST-plugin.</source>
        <translation>Clicca qui per mostrare o nascondere l&apos;interfaccia grafica (GUI) per i plugin VST.</translation>
    </message>
    <message>
        <source>Turn off all notes</source>
        <translation>Disabilita tutte le note</translation>
    </message>
    <message>
        <source>Open VST-plugin</source>
        <translation>Apri plugin VST</translation>
    </message>
    <message>
        <source>DLL-files (*.dll)</source>
        <translation>File DLL (*.dll)</translation>
    </message>
    <message>
        <source>EXE-files (*.exe)</source>
        <translation>File EXE (*.exe)</translation>
    </message>
    <message>
        <source>No VST-plugin loaded</source>
        <translation>Nessun plugin VST caricato</translation>
    </message>
    <message>
        <source>Control VST-plugin from LMMS host</source>
        <translation>Controlla il plugin VST dal terminale LMMS</translation>
    </message>
    <message>
        <source>Click here, if you want to control VST-plugin from host.</source>
        <translation>Cliccando qui, si apre una finestra di LMMS dove è possibile modificare le variabili del plugin VST.</translation>
    </message>
    <message>
        <source>Open VST-plugin preset</source>
        <translation>Apri un preset del plugin VST</translation>
    </message>
    <message>
        <source>Click here, if you want to open another *.fxp, *.fxb VST-plugin preset.</source>
        <translation>Cliccando qui, è possibile aprire un altro preset del plugin VST (*fxp, *fxb).</translation>
    </message>
    <message>
        <source>Previous (-)</source>
        <translation>Precedente (-)</translation>
    </message>
    <message>
        <source>Click here, if you want to switch to another VST-plugin preset program.</source>
        <translation>Cliccando qui, viene cambiato il preset del plugin VST.</translation>
    </message>
    <message>
        <source>Save preset</source>
        <translation>Salva il preset</translation>
    </message>
    <message>
        <source>Click here, if you want to save current VST-plugin preset program.</source>
        <translation>Cliccando qui è possibile salvare il preset corrente del plugin VST.</translation>
    </message>
    <message>
        <source>Next (+)</source>
        <translation>Successivo (+)</translation>
    </message>
    <message>
        <source>Click here to select presets that are currently loaded in VST.</source>
        <translation>Cliccando qui, è possibile selezionare i preset che sono caricati nel VST al momento.</translation>
    </message>
    <message>
        <source>Preset</source>
        <translation>Preset</translation>
    </message>
    <message>
        <source>by </source>
        <translation>da </translation>
    </message>
    <message>
        <source> - VST plugin control</source>
        <translation> - Controllo del plugin VST</translation>
    </message>
</context>
<context>
    <name>VstEffectControlDialog</name>
    <message>
        <source>Show/hide</source>
        <translation>Mostra/nascondi</translation>
    </message>
    <message>
        <source>Control VST-plugin from LMMS host</source>
        <translation>Controlla il plugin VST dal terminale LMMS</translation>
    </message>
    <message>
        <source>Click here, if you want to control VST-plugin from host.</source>
        <translation>Cliccando qui, si apre una finestra di LMMS dove è possibile modificare le variabili del plugin VST.</translation>
    </message>
    <message>
        <source>Open VST-plugin preset</source>
        <translation>Apri un preset del plugin VST</translation>
    </message>
    <message>
        <source>Click here, if you want to open another *.fxp, *.fxb VST-plugin preset.</source>
        <translation>Cliccando qui, è possibile aprire un altro preset del plugin VST (*fxp, *fxb).</translation>
    </message>
    <message>
        <source>Previous (-)</source>
        <translation>Precedente (-)</translation>
    </message>
    <message>
        <source>Click here, if you want to switch to another VST-plugin preset program.</source>
        <translation>Cliccando qui, viene cambiato il preset del plugin VST.</translation>
    </message>
    <message>
        <source>Next (+)</source>
        <translation>Successivo (+)</translation>
    </message>
    <message>
        <source>Click here to select presets that are currently loaded in VST.</source>
        <translation>Cliccando qui, è possibile selezionare i preset che sono caricati nel VST al momento.</translation>
    </message>
    <message>
        <source>Save preset</source>
        <translation>Salva il preset</translation>
    </message>
    <message>
        <source>Click here, if you want to save current VST-plugin preset program.</source>
        <translation>Cliccando qui è possibile salvare il preset corrente del plugin VST.</translation>
    </message>
    <message>
        <source>Effect by: </source>
        <translation>Effetto da: </translation>
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
        <translation>Caricamento plugin</translation>
    </message>
    <message>
        <source>Please wait while loading VST-plugin...</source>
        <translation>Attendere, caricamento del plugin VST...</translation>
    </message>
    <message>
        <source>Failed loading VST-plugin</source>
        <translation>Errore nel caricamento del plugin VST</translation>
    </message>
    <message>
        <source>The VST-plugin %1 could not be loaded for some reason.
If it runs with other VST-software under Linux, please contact an LMMS-developer!</source>
        <translation>Non è stato possibile caricare il plugin VST %1 a causa di alcuni errori.
Se, con altre applicazioni GNU/Linux, il plugin funziona, si prega di contattare uno sviluppatore di LMMS!</translation>
    </message>
    <message>
        <source>Open Preset</source>
        <translation>Apri preset</translation>
    </message>
    <message>
        <source>Vst Plugin Preset (*.fxp *.fxb)</source>
        <translation>Preset di plugin VST (*.fxp *.fxb)</translation>
    </message>
    <message>
        <source>: default</source>
        <translation>: default</translation>
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
        <translation>Salva Preset</translation>
    </message>
    <message>
        <source>.fxp</source>
        <translation>.fxp</translation>
    </message>
    <message>
        <source>.FXP</source>
        <translation>.FXP</translation>
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
        <translation></translation>
    </message>
    <message>
        <source>Filter Frequency</source>
        <translation>Frequenza del filtro</translation>
    </message>
    <message>
        <source>Filter Resonance</source>
        <translation>Risonanza del filtro</translation>
    </message>
    <message>
        <source>Bandwidth</source>
        <translation></translation>
    </message>
    <message>
        <source>FM Gain</source>
        <translation>Guadagno FM</translation>
    </message>
    <message>
        <source>Resonance Center Frequency</source>
        <translation>Frequenza Centrale di Risonanza</translation>
    </message>
    <message>
        <source>Resonance Bandwidth</source>
        <translation>Bandwidth di Risonanza</translation>
    </message>
    <message>
        <source>Forward MIDI Control Change Events</source>
        <translation>Inoltra segnali dai controlli MIDI</translation>
    </message>
</context>
<context>
    <name>ZynAddSubFxView</name>
    <message>
        <source>Show GUI</source>
        <translation>Mostra GUI</translation>
    </message>
    <message>
        <source>Click here to show or hide the graphical user interface (GUI) of ZynAddSubFX.</source>
        <translation>Clicca qui per mostrare o nascondere l&apos;interfaccia grafica (GUI) di ZynAddSubFX.</translation>
    </message>
    <message>
        <source>Portamento:</source>
        <translation></translation>
    </message>
    <message>
        <source>PORT</source>
        <translation></translation>
    </message>
    <message>
        <source>Filter Frequency:</source>
        <translation>Frequenza del Filtro:</translation>
    </message>
    <message>
        <source>FREQ</source>
        <translation>FREQ</translation>
    </message>
    <message>
        <source>Filter Resonance:</source>
        <translation>Risonanza del Filtro:</translation>
    </message>
    <message>
        <source>RES</source>
        <translation>RIS</translation>
    </message>
    <message>
        <source>Bandwidth:</source>
        <translation></translation>
    </message>
    <message>
        <source>BW</source>
        <translation></translation>
    </message>
    <message>
        <source>FM Gain:</source>
        <translation>Guadagno FM:</translation>
    </message>
    <message>
        <source>FM GAIN</source>
        <translation>GUAD FM</translation>
    </message>
    <message>
        <source>Resonance center frequency:</source>
        <translation>Frequenza Centrale di Risonanza:</translation>
    </message>
    <message>
        <source>RES CF</source>
        <translation>FC RIS</translation>
    </message>
    <message>
        <source>Resonance bandwidth:</source>
        <translation>Bandwidth della Risonanza:</translation>
    </message>
    <message>
        <source>RES BW</source>
        <translation>BW RIS</translation>
    </message>
    <message>
        <source>Forward MIDI Control Changes</source>
        <translation>Inoltra cambiamenti dai controlli MIDI</translation>
    </message>
</context>
<context>
    <name>audioFileProcessor</name>
    <message>
        <source>Reverse sample</source>
        <translation>Inverti il campione</translation>
    </message>
    <message>
        <source>Amplify</source>
        <translation>Amplificazione</translation>
    </message>
    <message>
        <source>Start of sample</source>
        <translation>Inizio del campione</translation>
    </message>
    <message>
        <source>End of sample</source>
        <translation>Fine del campione</translation>
    </message>
    <message>
        <source>Loop</source>
        <translation>Ripetizione</translation>
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
        <translation>FREQ</translation>
    </message>
    <message>
        <source>Frequency:</source>
        <translation>Frequenza:</translation>
    </message>
    <message>
        <source>GAIN</source>
        <translation>GUAD</translation>
    </message>
    <message>
        <source>Gain:</source>
        <translation>Guadagno:</translation>
    </message>
    <message>
        <source>RATIO</source>
        <translation>LIV</translation>
    </message>
    <message>
        <source>Ratio:</source>
        <translation>Livello:</translation>
    </message>
</context>
<context>
    <name>bassBoosterControls</name>
    <message>
        <source>Frequency</source>
        <translation>Frequenza</translation>
    </message>
    <message>
        <source>Gain</source>
        <translation>Guadagno</translation>
    </message>
    <message>
        <source>Ratio</source>
        <translation>Livello</translation>
    </message>
</context>
<context>
    <name>bbEditor</name>
    <message>
        <source>Play/pause current beat/bassline (Space)</source>
        <translation>Riproduci/metti in pausa il beat/bassline selezionato (Spazio)</translation>
    </message>
    <message>
        <source>Beat+Bassline Editor</source>
        <translation>Beat+Bassline Editor</translation>
    </message>
    <message>
        <source>Add beat/bassline</source>
        <translation>Aggiungi beat/bassline</translation>
    </message>
    <message>
        <source>Add automation-track</source>
        <translation>Aggiungi una traccia di automazione</translation>
    </message>
    <message>
        <source>Stop playback of current beat/bassline (Space)</source>
        <translation>Ferma il beat/bassline attuale (Spazio)</translation>
    </message>
    <message>
        <source>Click here to play the current beat/bassline.  The beat/bassline is automatically looped when its end is reached.</source>
        <translation>Cliccando qui si riprodurre il beat/bassline selezionato. Il beat/bassline ricomincia automaticamente quando finisce.</translation>
    </message>
    <message>
        <source>Click here to stop playing of current beat/bassline.</source>
        <translation>Cliccando qui si ferma la riproduzione del beat/bassline attivo.</translation>
    </message>
    <message>
        <source>Remove steps</source>
        <translation>Elimina note</translation>
    </message>
    <message>
        <source>Add steps</source>
        <translation>Aggiungi note</translation>
    </message>
</context>
<context>
    <name>bbTCOView</name>
    <message>
        <source>Open in Beat+Bassline-Editor</source>
        <translation>Apri nell&apos;editor di Beat+Bassline</translation>
    </message>
    <message>
        <source>Reset name</source>
        <translation>Reimposta nome</translation>
    </message>
    <message>
        <source>Change name</source>
        <translation>Cambia nome</translation>
    </message>
    <message>
        <source>Change color</source>
        <translation>Cambia colore</translation>
    </message>
</context>
<context>
    <name>bbTrack</name>
    <message>
        <source>Beat/Bassline %1</source>
        <translation>Beat/Bassline %1</translation>
    </message>
    <message>
        <source>Clone of %1</source>
        <translation>Clone di %1</translation>
    </message>
</context>
<context>
    <name>bitInvader</name>
    <message>
        <source>Samplelength</source>
        <translation>LunghezzaCampione</translation>
    </message>
</context>
<context>
    <name>bitInvaderView</name>
    <message>
        <source>Sample Length</source>
        <translation>Lunghezza del campione</translation>
    </message>
    <message>
        <source>Sine wave</source>
        <translation>Onda sinusoidale</translation>
    </message>
    <message>
        <source>Triangle wave</source>
        <translation>Onda triangolare</translation>
    </message>
    <message>
        <source>Saw wave</source>
        <translation>Onda a dente di sega</translation>
    </message>
    <message>
        <source>Square wave</source>
        <translation>Onda quadra</translation>
    </message>
    <message>
        <source>White noise wave</source>
        <translation>Rumore bianco</translation>
    </message>
    <message>
        <source>User defined wave</source>
        <translation>Forma d&apos;onda personalizzata</translation>
    </message>
    <message>
        <source>Smooth</source>
        <translation>Ammorbidisci</translation>
    </message>
    <message>
        <source>Click here to smooth waveform.</source>
        <translation>Cliccando qui la forma d&apos;onda viene ammorbidita.</translation>
    </message>
    <message>
        <source>Interpolation</source>
        <translation>Interpolazione</translation>
    </message>
    <message>
        <source>Normalize</source>
        <translation>Normalizza</translation>
    </message>
    <message>
        <source>Draw your own waveform here by dragging your mouse on this graph.</source>
        <translation>Cliccando e trascinando il mouse in questo grafico è possibile disegnare una forma d&apos;onda personalizzata.</translation>
    </message>
    <message>
        <source>Click for a sine-wave.</source>
        <translation>Cliccando qui si ottiene una forma d&apos;onda sinusoidale.</translation>
    </message>
    <message>
        <source>Click here for a triangle-wave.</source>
        <translation>Cliccando qui si ottiene un&apos;onda triangolare.</translation>
    </message>
    <message>
        <source>Click here for a saw-wave.</source>
        <translation>Cliccando qui si ottiene un&apos;onda a dente di sega.</translation>
    </message>
    <message>
        <source>Click here for a square-wave.</source>
        <translation>Cliccando qui si ottiene un&apos;onda quadra.</translation>
    </message>
    <message>
        <source>Click here for white-noise.</source>
        <translation>Cliccando qui si ottiene rumore bianco.</translation>
    </message>
    <message>
        <source>Click here for a user-defined shape.</source>
        <translation>Cliccando qui è possibile definire una forma d&apos;onda personalizzata.</translation>
    </message>
</context>
<context>
    <name>exportProjectDialog</name>
    <message>
        <source>Could not open file</source>
        <translation>Non è stato possibile aprire il file</translation>
    </message>
    <message>
        <source>Could not open file %1 for writing.
Please make sure you have write-permission to the file and the directory containing the file and try again!</source>
        <translation>Impossibile aprire in scrittura il file %1.
Assicurarsi di avere i permessi in scrittura per il file e per la directory contenente il file e riprovare!</translation>
    </message>
    <message>
        <source>Error</source>
        <translation>Errore</translation>
    </message>
    <message>
        <source>Error while determining file-encoder device. Please try to choose a different output format.</source>
        <translation>Si è verificato un errore nel tentativo di determinare il dispositivo per la codifica del file. Si prega di selezionare un formato differente.</translation>
    </message>
    <message>
        <source>Rendering: %1%</source>
        <translation>Renderizzazione: %1%</translation>
    </message>
    <message>
        <source>Export project to %1</source>
        <translation>Esporta il progetto in %1</translation>
    </message>
</context>
<context>
    <name>fader</name>
    <message>
        <source>Please enter a new value between %1 and %2:</source>
        <translation>Inserire un valore compreso tra %1 e %2:</translation>
    </message>
</context>
<context>
    <name>fileBrowser</name>
    <message>
        <source>Browser</source>
        <translation>Browser</translation>
    </message>
</context>
<context>
    <name>fileBrowserTreeWidget</name>
    <message>
        <source>Send to active instrument-track</source>
        <translation>Sostituisci questo strumento alla traccia attiva</translation>
    </message>
    <message>
        <source>Open in new instrument-track/Song-Editor</source>
        <translation>Usa in una nuova traccia nel Song-Editor</translation>
    </message>
    <message>
        <source>Open in new instrument-track/B+B Editor</source>
        <translation>Usa in una nuova traccia nel B+B Editor</translation>
    </message>
    <message>
        <source>Loading sample</source>
        <translation>Caricamento campione</translation>
    </message>
    <message>
        <source>Please wait, loading sample for preview...</source>
        <translation>Attendere, stiamo caricando il file per l&apos;anteprima...</translation>
    </message>
    <message>
        <source>--- Factory files ---</source>
        <translation>--- File di fabbrica ---</translation>
    </message>
</context>
<context>
    <name>graphModel</name>
    <message>
        <source>Graph</source>
        <translation>Grafico</translation>
    </message>
</context>
<context>
    <name>kickerInstrument</name>
    <message>
        <source>Start frequency</source>
        <translation>Frequenza iniziale</translation>
    </message>
    <message>
        <source>End frequency</source>
        <translation>Frequenza finale</translation>
    </message>
    <message>
        <source>Decay</source>
        <translation>Decadimento</translation>
    </message>
    <message>
        <source>Distortion</source>
        <translation>Distorsione</translation>
    </message>
    <message>
        <source>Gain</source>
        <translation>Guadagno</translation>
    </message>
</context>
<context>
    <name>kickerInstrumentView</name>
    <message>
        <source>Start frequency:</source>
        <translation>Frequenza iniziale:</translation>
    </message>
    <message>
        <source>End frequency:</source>
        <translation>Frequenza finale:</translation>
    </message>
    <message>
        <source>Decay:</source>
        <translation>Decadimento:</translation>
    </message>
    <message>
        <source>Distortion:</source>
        <translation>Distorsione:</translation>
    </message>
    <message>
        <source>Gain:</source>
        <translation>Guadagno:</translation>
    </message>
</context>
<context>
    <name>knob</name>
    <message>
        <source>&amp;Help</source>
        <translation>&amp;Aiuto</translation>
    </message>
    <message>
        <source>Please enter a new value between %1 and %2:</source>
        <translation>Inserire un valore compreso tra %1 e %2:</translation>
    </message>
    <message>
        <source>Please enter a new value between -96.0 dBV and 6.0 dBV:</source>
        <translation>Inserire un nuovo valore tra -96.0 dBV e 6.0 dBV:</translation>
    </message>
</context>
<context>
    <name>ladspaBrowserView</name>
    <message>
        <source>Available Effects</source>
        <translation>Effetti disponibili</translation>
    </message>
    <message>
        <source>Unavailable Effects</source>
        <translation>Effetti non disponibili</translation>
    </message>
    <message>
        <source>Instruments</source>
        <translation>Strumenti</translation>
    </message>
    <message>
        <source>Analysis Tools</source>
        <translation>Strumenti di analisi</translation>
    </message>
    <message>
        <source>Don&apos;t know</source>
        <translation>Sconosciuto</translation>
    </message>
    <message>
        <source>This dialog displays information on all of the LADSPA plugins LMMS was able to locate. The plugins are divided into five categories based upon an interpretation of the port types and names.

Available Effects are those that can be used by LMMS. In order for LMMS to be able to use an effect, it must, first and foremost, be an effect, which is to say, it has to have both input channels and output channels. LMMS identifies an input channel as an audio rate port containing &apos;in&apos; in the name. Output channels are identified by the letters &apos;out&apos;. Furthermore, the effect must have the same number of inputs and outputs and be real time capable.

Unavailable Effects are those that were identified as effects, but either didn&apos;t have the same number of input and output channels or weren&apos;t real time capable.

Instruments are plugins for which only output channels were identified.

Analysis Tools are plugins for which only input channels were identified.

Don&apos;t Knows are plugins for which no input or output channels were identified.

Double clicking any of the plugins will bring up information on the ports.</source>
        <translation>Questa finestra mostra le informazioni relative ai plugin LADSPA che LMMS è stato in grado di localizzare. I plugin sono divisi in cinque categorie, in base all&apos;interpretazione dei tipi di porta e ai nomi.

Gli Effetti Disponibili sono quelli che possono essere usati con LMMS. Perché LMMS possa usare un effetto deve, prima di tutto, essere un effetto, cioè deve avere sia ingressi che uscite. LMMS identifica un ingresso come una porta con una frequenza audio associata e che contiene &apos;in&apos; nel nome. Le uscite sono identificate dalle lettere &apos;out&apos;. Inoltre, l&apos;effetto deve avere lo stesso numero di ingressi e di uscite e deve poter funzionare in real time.

Gli Effetti non Disponibili sono quelli che sono stati identificati come effetti ma che non hanno lo stesso numero di ingressi e uscite oppure non supportano il real time.

Gli Strumenti sono plugin che hanno solo uscite.

Gli Strumenti di Analisi sono plugin che hanno solo ingressi.

Quelli Sconosciuti sono plugin che non hanno né ingressi né uscite.

Facendo doppio click sui plugin verranno fornite informazioni sulle relative porte.</translation>
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
        <translation>Plugin</translation>
    </message>
    <message>
        <source>Description</source>
        <translation>Descrizione</translation>
    </message>
</context>
<context>
    <name>ladspaPortDialog</name>
    <message>
        <source>Ports</source>
        <translation>Porte</translation>
    </message>
    <message>
        <source>Name</source>
        <translation>Nome</translation>
    </message>
    <message>
        <source>Rate</source>
        <translation>Frequenza</translation>
    </message>
    <message>
        <source>Direction</source>
        <translation>Direzione</translation>
    </message>
    <message>
        <source>Type</source>
        <translation>Tipo</translation>
    </message>
    <message>
        <source>Min &lt; Default &lt; Max</source>
        <translation>Minimo &lt; Predefinito &lt; Massimo</translation>
    </message>
    <message>
        <source>Logarithmic</source>
        <translation>Logaritmico</translation>
    </message>
    <message>
        <source>SR Dependent</source>
        <translation>Dipendente da SR</translation>
    </message>
    <message>
        <source>Control</source>
        <translation>Controllo</translation>
    </message>
    <message>
        <source>Input</source>
        <translation>Ingresso</translation>
    </message>
    <message>
        <source>Output</source>
        <translation>Uscita</translation>
    </message>
    <message>
        <source>Toggled</source>
        <translation>Abilitato</translation>
    </message>
    <message>
        <source>Integer</source>
        <translation>Intero</translation>
    </message>
    <message>
        <source>Float</source>
        <translation>Virgola mobile</translation>
    </message>
    <message>
        <source>Yes</source>
        <translation>Sì</translation>
    </message>
    <message>
        <source>Audio</source>
        <translation>Audio</translation>
    </message>
</context>
<context>
    <name>lb302Synth</name>
    <message>
        <source>VCF Cutoff Frequency</source>
        <translation>VCF - frequenza di taglio</translation>
    </message>
    <message>
        <source>VCF Resonance</source>
        <translation>VCF - Risonanza</translation>
    </message>
    <message>
        <source>VCF Envelope Mod</source>
        <translation>VCF  - modulazione dell&apos;envelope</translation>
    </message>
    <message>
        <source>VCF Envelope Decay</source>
        <translation>VCF - decadimento dell&apos;envelope</translation>
    </message>
    <message>
        <source>Distortion</source>
        <translation>Distorsione</translation>
    </message>
    <message>
        <source>Waveform</source>
        <translation>Forma d&apos;onda</translation>
    </message>
    <message>
        <source>Slide Decay</source>
        <translation>Decadimento slide</translation>
    </message>
    <message>
        <source>Slide</source>
        <translation>Slide</translation>
    </message>
    <message>
        <source>Accent</source>
        <translation>Accento</translation>
    </message>
    <message>
        <source>Dead</source>
        <translation>Dead</translation>
    </message>
    <message>
        <source>24dB/oct Filter</source>
        <translation>Filtro 24dB/ottava</translation>
    </message>
</context>
<context>
    <name>lb302SynthView</name>
    <message>
        <source>Cutoff Freq:</source>
        <translation>Freq. di taglio:</translation>
    </message>
    <message>
        <source>Resonance:</source>
        <translation>Risonanza:</translation>
    </message>
    <message>
        <source>Env Mod:</source>
        <translation>Env Mod:</translation>
    </message>
    <message>
        <source>Decay:</source>
        <translation>Decadimento:</translation>
    </message>
    <message>
        <source>303-es-que, 24dB/octave, 3 pole filter</source>
        <translation>filtro tripolare &quot;tipo 303&quot;, 24dB/ottava</translation>
    </message>
    <message>
        <source>Slide Decay:</source>
        <translation>Decadimento slide:</translation>
    </message>
    <message>
        <source>DIST:</source>
        <translation>DIST:</translation>
    </message>
    <message>
        <source>Saw wave</source>
        <translation>Onda a dente di sega</translation>
    </message>
    <message>
        <source>Click here for a saw-wave.</source>
        <translation>Cliccando qui si ottiene un&apos;onda a dente di sega.</translation>
    </message>
    <message>
        <source>Triangle wave</source>
        <translation>Onda triangolare</translation>
    </message>
    <message>
        <source>Click here for a triangle-wave.</source>
        <translation>Cliccando qui si ottiene un&apos;onda triangolare.</translation>
    </message>
    <message>
        <source>Square wave</source>
        <translation>Onda quadra</translation>
    </message>
    <message>
        <source>Click here for a square-wave.</source>
        <translation>Cliccando qui si ottiene un&apos;onda quadra.</translation>
    </message>
    <message>
        <source>Rounded square wave</source>
        <translation>Onda quadra arrotondata</translation>
    </message>
    <message>
        <source>Click here for a square-wave with a rounded end.</source>
        <translation>Cliccando qui si ottiene un&apos;onda quadra arrotondata.</translation>
    </message>
    <message>
        <source>Moog wave</source>
        <translation>Onda moog</translation>
    </message>
    <message>
        <source>Click here for a moog-like wave.</source>
        <translation>Cliccando qui si ottieme un&apos;onda moog.</translation>
    </message>
    <message>
        <source>Sine wave</source>
        <translation>Onda sinusoidale</translation>
    </message>
    <message>
        <source>Click for a sine-wave.</source>
        <translation>Cliccando qui si ottiene una forma d&apos;onda sinusoidale.</translation>
    </message>
    <message>
        <source>White noise wave</source>
        <translation>Rumore bianco</translation>
    </message>
    <message>
        <source>Click here for an exponential wave.</source>
        <translation>Cliccando qui si ha un&apos;onda esponenziale.</translation>
    </message>
    <message>
        <source>Click here for white-noise.</source>
        <translation>Cliccando qui si ottiene rumore bianco.</translation>
    </message>
</context>
<context>
    <name>lb303Synth</name>
    <message>
        <source>VCF Cutoff Frequency</source>
        <translation>VCF - frequenza di taglio</translation>
    </message>
    <message>
        <source>VCF Resonance</source>
        <translation>VCF - Risonanza</translation>
    </message>
    <message>
        <source>VCF Envelope Mod</source>
        <translation>VCF  - modulazione dell&apos;envelope</translation>
    </message>
    <message>
        <source>VCF Envelope Decay</source>
        <translation>VCF - decadimento dell&apos;envelope</translation>
    </message>
    <message>
        <source>Distortion</source>
        <translation>Distorsione</translation>
    </message>
    <message>
        <source>Waveform</source>
        <translation>Forma d&apos;onda</translation>
    </message>
    <message>
        <source>Slide Decay</source>
        <translation>Decadimento slide</translation>
    </message>
    <message>
        <source>Slide</source>
        <translation>Slide</translation>
    </message>
    <message>
        <source>Accent</source>
        <translation>Accento</translation>
    </message>
    <message>
        <source>Dead</source>
        <translation>Dead</translation>
    </message>
    <message>
        <source>24dB/oct Filter</source>
        <translation>Filtro 24dB/ottava</translation>
    </message>
</context>
<context>
    <name>lb303SynthView</name>
    <message>
        <source>Cutoff Freq:</source>
        <translation>Freq. di taglio:</translation>
    </message>
    <message>
        <source>CUT</source>
        <translation>TAG</translation>
    </message>
    <message>
        <source>Resonance:</source>
        <translation>Risonanza:</translation>
    </message>
    <message>
        <source>RES</source>
        <translation>RIS</translation>
    </message>
    <message>
        <source>Env Mod:</source>
        <translation>Env Mod:</translation>
    </message>
    <message>
        <source>ENV MOD</source>
        <translation>ENV MOD</translation>
    </message>
    <message>
        <source>Decay:</source>
        <translation>Decadimento:</translation>
    </message>
    <message>
        <source>DEC</source>
        <translation>DEC</translation>
    </message>
    <message>
        <source>303-es-que, 24dB/octave, 3 pole filter</source>
        <translation>filtro tripolare &quot;tipo 303&quot;, 24dB/ottava</translation>
    </message>
    <message>
        <source>Slide Decay:</source>
        <translation>Decadimento slide:</translation>
    </message>
    <message>
        <source>SLIDE</source>
        <translation>SLIDE</translation>
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
        <translation>Durezza</translation>
    </message>
    <message>
        <source>Position</source>
        <translation>Posizione</translation>
    </message>
    <message>
        <source>Vibrato Gain</source>
        <translation>Guadagno del vibrato</translation>
    </message>
    <message>
        <source>Vibrato Freq</source>
        <translation>Fequenza del vibrato</translation>
    </message>
    <message>
        <source>Modulator</source>
        <translation>Modulatore</translation>
    </message>
    <message>
        <source>LFO Speed</source>
        <translation>Velocità dell&apos;LFO</translation>
    </message>
    <message>
        <source>LFO Depth</source>
        <translation>Profondità dell&apos;LFO</translation>
    </message>
    <message>
        <source>Pressure</source>
        <translation>Pressione</translation>
    </message>
    <message>
        <source>Motion</source>
        <translation>Moto</translation>
    </message>
    <message>
        <source>Speed</source>
        <translation>Velocità</translation>
    </message>
    <message>
        <source>Spread</source>
        <translation>Apertura</translation>
    </message>
    <message>
        <source>Stick Mix</source>
        <translation>Stick Mix</translation>
    </message>
    <message>
        <source>Crossfade</source>
        <translation>Crossfade</translation>
    </message>
    <message>
        <source>ADSR</source>
        <translation>ADSR</translation>
    </message>
    <message>
        <source>Bowed</source>
        <translation>Bowed</translation>
    </message>
    <message>
        <source>Missing files</source>
        <translation>File mancanti</translation>
    </message>
    <message>
        <source>Your Stk-installation seems to be incomplete. Please make sure the full Stk-package is installed!</source>
        <translation>L&apos;installazione di Stk sembra incompleta. Assicurarsi che sia installato il pacchetto Stk completo!</translation>
    </message>
    <message>
        <source>Marimba</source>
        <translation>Marimba</translation>
    </message>
    <message>
        <source>Vibraphone</source>
        <translation>Vibraphone</translation>
    </message>
    <message>
        <source>Agogo</source>
        <translation>Agogo</translation>
    </message>
    <message>
        <source>Wood1</source>
        <translation>Legno1</translation>
    </message>
    <message>
        <source>Reso</source>
        <translation>Reso</translation>
    </message>
    <message>
        <source>Wood2</source>
        <translation>Legno2</translation>
    </message>
    <message>
        <source>Beats</source>
        <translation>Beats</translation>
    </message>
    <message>
        <source>Two Fixed</source>
        <translation>Two Fixed</translation>
    </message>
    <message>
        <source>Clump</source>
        <translation>Clump</translation>
    </message>
    <message>
        <source>Tubular Bells</source>
        <translation>Tubular Bells</translation>
    </message>
    <message>
        <source>Uniform Bar</source>
        <translation>Uniform Bar</translation>
    </message>
    <message>
        <source>Tuned Bar</source>
        <translation>Tuned Bar</translation>
    </message>
    <message>
        <source>Glass</source>
        <translation>Glass</translation>
    </message>
    <message>
        <source>Tibetan Bowl</source>
        <translation>Tibetan Bowl</translation>
    </message>
</context>
<context>
    <name>malletsInstrumentView</name>
    <message>
        <source>Instrument</source>
        <translation>Strumento</translation>
    </message>
    <message>
        <source>Spread</source>
        <translation>Apertura</translation>
    </message>
    <message>
        <source>Spread:</source>
        <translation>Apertura:</translation>
    </message>
    <message>
        <source>Hardness</source>
        <translation>Durezza</translation>
    </message>
    <message>
        <source>Hardness:</source>
        <translation>Durezza:</translation>
    </message>
    <message>
        <source>Position</source>
        <translation>Posizione</translation>
    </message>
    <message>
        <source>Position:</source>
        <translation>Posizione:</translation>
    </message>
    <message>
        <source>Vib Gain</source>
        <translation>Guad Vib</translation>
    </message>
    <message>
        <source>Vib Gain:</source>
        <translation>Guad Vib:</translation>
    </message>
    <message>
        <source>Vib Freq</source>
        <translation>Freq Vib</translation>
    </message>
    <message>
        <source>Vib Freq:</source>
        <translation>Freq Vib:</translation>
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
        <translation>Modulatore</translation>
    </message>
    <message>
        <source>Modulator:</source>
        <translation>Modulatore:</translation>
    </message>
    <message>
        <source>Crossfade</source>
        <translation>Crossfade</translation>
    </message>
    <message>
        <source>Crossfade:</source>
        <translation>Crossfade:</translation>
    </message>
    <message>
        <source>LFO Speed</source>
        <translation>Velocità LFO</translation>
    </message>
    <message>
        <source>LFO Speed:</source>
        <translation>Velocità LFO:</translation>
    </message>
    <message>
        <source>LFO Depth</source>
        <translation>Profondità LFO</translation>
    </message>
    <message>
        <source>LFO Depth:</source>
        <translation>Profondità LFO:</translation>
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
        <source>Pressure</source>
        <translation>Pressione</translation>
    </message>
    <message>
        <source>Pressure:</source>
        <translation>Pressione:</translation>
    </message>
    <message>
        <source>Motion</source>
        <translation>Moto</translation>
    </message>
    <message>
        <source>Motion:</source>
        <translation>Moto:</translation>
    </message>
    <message>
        <source>Speed</source>
        <translation>Velocità</translation>
    </message>
    <message>
        <source>Speed:</source>
        <translation>Velocità:</translation>
    </message>
    <message>
        <source>Bowed</source>
        <translation>Bowed</translation>
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
        <translation> - Controllo parametri VST</translation>
    </message>
    <message>
        <source>VST Sync</source>
        <translation>Sync VST</translation>
    </message>
    <message>
        <source>Click here if you want to synchronize all parameters with VST plugin.</source>
        <translation>Clicca qui se vuoi sincronizzare tutti i parametri con il plugin VST.</translation>
    </message>
    <message>
        <source>Automated</source>
        <translation>Automatizzati</translation>
    </message>
    <message>
        <source>Click here if you want to display automated parameters only.</source>
        <translation>Clicca qui se vuoi visualizzare solo i parametri automatizzati.</translation>
    </message>
    <message>
        <source>    Close    </source>
        <translation>    Chiudi    </translation>
    </message>
    <message>
        <source>Close VST effect knob-controller window.</source>
        <translation>Chiudi la finestra delle manopole dell&apos;effetto VST.</translation>
    </message>
</context>
<context>
    <name>manageVestigeInstrumentView</name>
    <message>
        <source> - VST plugin control</source>
        <translation> - Controllo del plugin VST</translation>
    </message>
    <message>
        <source>VST Sync</source>
        <translation>Sync VST</translation>
    </message>
    <message>
        <source>Click here if you want to synchronize all parameters with VST plugin.</source>
        <translation>Clicca qui se vuoi sincronizzare tutti i parametri con il plugin VST.</translation>
    </message>
    <message>
        <source>Automated</source>
        <translation>Automatizzati</translation>
    </message>
    <message>
        <source>Click here if you want to display automated parameters only.</source>
        <translation>Clicca qui se vuoi visualizzare solo i parametri automatizzati.</translation>
    </message>
    <message>
        <source>    Close    </source>
        <translation>    Chiudi    </translation>
    </message>
    <message>
        <source>Close VST plugin knob-controller window.</source>
        <translation>Chiudi la finestra delle manopole del plugin VST.</translation>
    </message>
</context>
<context>
    <name>nineButtonSelector</name>
    <message>
        <source>&amp;Help</source>
        <translation>&amp;Aiuto</translation>
    </message>
</context>
<context>
    <name>opl2instrument</name>
    <message>
        <source>Patch</source>
        <translation>Patch</translation>
    </message>
    <message>
        <source>Op 1 Attack</source>
        <translation>Attacco Op 1</translation>
    </message>
    <message>
        <source>Op 1 Decay</source>
        <translation>Decadimento Op 1</translation>
    </message>
    <message>
        <source>Op 1 Sustain</source>
        <translation>Sostegno Op 1</translation>
    </message>
    <message>
        <source>Op 1 Release</source>
        <translation>Rilascio Op 1</translation>
    </message>
    <message>
        <source>Op 1 Level</source>
        <translation>Livello  Op 1</translation>
    </message>
    <message>
        <source>Op 1 Level Scaling</source>
        <translation>Scala di livello Op 1</translation>
    </message>
    <message>
        <source>Op 1 Frequency Multiple</source>
        <translation>Moltiplicatore di frequenza Op 1</translation>
    </message>
    <message>
        <source>Op 1 Feedback</source>
        <translation>Feedback Op 1</translation>
    </message>
    <message>
        <source>Op 1 Key Scaling Rate</source>
        <translation>Rateo del Key Scaling Op 1</translation>
    </message>
    <message>
        <source>Op 1 Percussive Envelope</source>
        <translation>Envelope a percussione Op 1</translation>
    </message>
    <message>
        <source>Op 1 Tremolo</source>
        <translation>Tremolo Op 1</translation>
    </message>
    <message>
        <source>Op 1 Vibrato</source>
        <translation>Vibrato Op 1</translation>
    </message>
    <message>
        <source>Op 1 Waveform</source>
        <translation>Forma d&apos;onda Op 1</translation>
    </message>
    <message>
        <source>Op 2 Attack</source>
        <translation>Attacco Op 2</translation>
    </message>
    <message>
        <source>Op 2 Decay</source>
        <translation>Decadimento Op 2</translation>
    </message>
    <message>
        <source>Op 2 Sustain</source>
        <translation>Sostegno Op 2</translation>
    </message>
    <message>
        <source>Op 2 Release</source>
        <translation>Rilascio Op 2</translation>
    </message>
    <message>
        <source>Op 2 Level</source>
        <translation>Livello  Op 2</translation>
    </message>
    <message>
        <source>Op 2 Level Scaling</source>
        <translation>Scala di livello Op 2</translation>
    </message>
    <message>
        <source>Op 2 Frequency Multiple</source>
        <translation>Moltiplicatore di frequenza Op 2</translation>
    </message>
    <message>
        <source>Op 2 Key Scaling Rate</source>
        <translation>Rateo del Key Scaling Op 2</translation>
    </message>
    <message>
        <source>Op 2 Percussive Envelope</source>
        <translation>Envelope a percussione Op 2</translation>
    </message>
    <message>
        <source>Op 2 Tremolo</source>
        <translation>Tremolo Op 2</translation>
    </message>
    <message>
        <source>Op 2 Vibrato</source>
        <translation>Vibrato Op 2</translation>
    </message>
    <message>
        <source>Op 2 Waveform</source>
        <translation>Forma d&apos;onda Op 2</translation>
    </message>
    <message>
        <source>FM</source>
        <translation>FM</translation>
    </message>
    <message>
        <source>Vibrato Depth</source>
        <translation>Profondità del Vibrato</translation>
    </message>
    <message>
        <source>Tremolo Depth</source>
        <translation>Profondità del Tremolo</translation>
    </message>
</context>
<context>
    <name>organicInstrument</name>
    <message>
        <source>Distortion</source>
        <translation>Distorsione</translation>
    </message>
    <message>
        <source>Volume</source>
        <translation>Volume</translation>
    </message>
</context>
<context>
    <name>organicInstrumentView</name>
    <message>
        <source>Distortion:</source>
        <translation>Distorsione:</translation>
    </message>
    <message>
        <source>Volume:</source>
        <translation>Volume:</translation>
    </message>
    <message>
        <source>Randomise</source>
        <translation>Rendi casuale</translation>
    </message>
    <message>
        <source>Osc %1 waveform:</source>
        <translation>Onda osc %1:</translation>
    </message>
    <message>
        <source>Osc %1 volume:</source>
        <translation>Volume osc %1:</translation>
    </message>
    <message>
        <source>Osc %1 panning:</source>
        <translation>Panning osc %1:</translation>
    </message>
    <message>
        <source>Osc %1 fine detuning left:</source>
        <translation>Intonazione precisa osc %1 sinistra:</translation>
    </message>
    <message>
        <source>cents</source>
        <translation>centesimi</translation>
    </message>
</context>
<context>
    <name>papuInstrument</name>
    <message>
        <source>Sweep time</source>
        <translation>Tempo di sweep</translation>
    </message>
    <message>
        <source>Sweep direction</source>
        <translation>Direzione sweep</translation>
    </message>
    <message>
        <source>Sweep RtShift amount</source>
        <translation>Quantità RtShift per lo sweep</translation>
    </message>
    <message>
        <source>Wave Pattern Duty</source>
        <translation>Wave Pattern Duty</translation>
    </message>
    <message>
        <source>Channel 1 volume</source>
        <translation>Volume del canale 1</translation>
    </message>
    <message>
        <source>Volume sweep direction</source>
        <translation>Direzione sweep del volume</translation>
    </message>
    <message>
        <source>Length of each step in sweep</source>
        <translation>Lunghezza di ogni passo nello sweep</translation>
    </message>
    <message>
        <source>Channel 2 volume</source>
        <translation>Volume del canale 2</translation>
    </message>
    <message>
        <source>Channel 3 volume</source>
        <translation>Volume del canale 3</translation>
    </message>
    <message>
        <source>Channel 4 volume</source>
        <translation>Volume del canale 4</translation>
    </message>
    <message>
        <source>Right Output level</source>
        <translation>Volume uscita destra</translation>
    </message>
    <message>
        <source>Left Output level</source>
        <translation>Volume uscita sinistra</translation>
    </message>
    <message>
        <source>Channel 1 to SO2 (Left)</source>
        <translation>Canale 1 a SO2 (sinistra)</translation>
    </message>
    <message>
        <source>Channel 2 to SO2 (Left)</source>
        <translation>Canale 2 a SO2 (sinistra)</translation>
    </message>
    <message>
        <source>Channel 3 to SO2 (Left)</source>
        <translation>Canale 3 a SO2 (sinistra)</translation>
    </message>
    <message>
        <source>Channel 4 to SO2 (Left)</source>
        <translation>Canale 4 a SO2 (sinistra)</translation>
    </message>
    <message>
        <source>Channel 1 to SO1 (Right)</source>
        <translation>Canale 1 a SO1 (destra)</translation>
    </message>
    <message>
        <source>Channel 2 to SO1 (Right)</source>
        <translation>Canale 2 a SO1 (destra)</translation>
    </message>
    <message>
        <source>Channel 3 to SO1 (Right)</source>
        <translation>Canale 3 a SO1 (destra)</translation>
    </message>
    <message>
        <source>Channel 4 to SO1 (Right)</source>
        <translation>Canale 4 a SO1 (destra)</translation>
    </message>
    <message>
        <source>Treble</source>
        <translation>Alti</translation>
    </message>
    <message>
        <source>Bass</source>
        <translation>Bassi</translation>
    </message>
    <message>
        <source>Shift Register width</source>
        <translation>Ampiezza spostamento del registro</translation>
    </message>
</context>
<context>
    <name>papuInstrumentView</name>
    <message>
        <source>Sweep Time:</source>
        <translation>Tempo di sweep:</translation>
    </message>
    <message>
        <source>Sweep Time</source>
        <translation>Tempo di sweep</translation>
    </message>
    <message>
        <source>Sweep RtShift amount:</source>
        <translation>Quantità RtShift per lo sweep:</translation>
    </message>
    <message>
        <source>Sweep RtShift amount</source>
        <translation>Quantità RtShift per lo sweep</translation>
    </message>
    <message>
        <source>Wave pattern duty:</source>
        <translation>Wave Pattern Duty:</translation>
    </message>
    <message>
        <source>Wave Pattern Duty</source>
        <translation>Wave Pattern Duty</translation>
    </message>
    <message>
        <source>Square Channel 1 Volume:</source>
        <translation>Volume del canale 1 square:</translation>
    </message>
    <message>
        <source>Length of each step in sweep:</source>
        <translation>Lunghezza di ogni passo nello sweep:</translation>
    </message>
    <message>
        <source>Length of each step in sweep</source>
        <translation>Lunghezza di ogni passo nello sweep</translation>
    </message>
    <message>
        <source>Wave pattern duty</source>
        <translation>Wave Pattern Duty</translation>
    </message>
    <message>
        <source>Square Channel 2 Volume:</source>
        <translation>Volume square del canale 2:</translation>
    </message>
    <message>
        <source>Square Channel 2 Volume</source>
        <translation>Volume square del canale 2</translation>
    </message>
    <message>
        <source>Wave Channel Volume:</source>
        <translation>Volume wave del canale:</translation>
    </message>
    <message>
        <source>Wave Channel Volume</source>
        <translation>Volume wave del canale:</translation>
    </message>
    <message>
        <source>Noise Channel Volume:</source>
        <translation>Volume rumore del canale:</translation>
    </message>
    <message>
        <source>Noise Channel Volume</source>
        <translation>Volume rumore del canale</translation>
    </message>
    <message>
        <source>SO1 Volume (Right):</source>
        <translation>Volume SO1 (destra):</translation>
    </message>
    <message>
        <source>SO1 Volume (Right)</source>
        <translation>Volume SO1 (destra)</translation>
    </message>
    <message>
        <source>SO2 Volume (Left):</source>
        <translation>Volume SO2 (sinistra):</translation>
    </message>
    <message>
        <source>SO2 Volume (Left)</source>
        <translation>Volume SO2 (sinistra)</translation>
    </message>
    <message>
        <source>Treble:</source>
        <translation>Alti:</translation>
    </message>
    <message>
        <source>Treble</source>
        <translation>Alti</translation>
    </message>
    <message>
        <source>Bass:</source>
        <translation>Bassi:</translation>
    </message>
    <message>
        <source>Bass</source>
        <translation>Bassi</translation>
    </message>
    <message>
        <source>Sweep Direction</source>
        <translation>Direzione sweep</translation>
    </message>
    <message>
        <source>Volume Sweep Direction</source>
        <translation>Direzione sweep del volume</translation>
    </message>
    <message>
        <source>Shift Register Width</source>
        <translation>Ampiezza spostamento del registro</translation>
    </message>
    <message>
        <source>Channel1 to SO1 (Right)</source>
        <translation>Canale 1 a SO1 (destra)</translation>
    </message>
    <message>
        <source>Channel2 to SO1 (Right)</source>
        <translation>Canale 2 a SO1 (destra)</translation>
    </message>
    <message>
        <source>Channel3 to SO1 (Right)</source>
        <translation>Canale 3 a SO1 (destra)</translation>
    </message>
    <message>
        <source>Channel4 to SO1 (Right)</source>
        <translation>Canale 4 a SO1 (destra)</translation>
    </message>
    <message>
        <source>Channel1 to SO2 (Left)</source>
        <translation>Canale 1 a SO2 (sinistra)</translation>
    </message>
    <message>
        <source>Channel2 to SO2 (Left)</source>
        <translation>Canale 1 a SO2 (sinistra)</translation>
    </message>
    <message>
        <source>Channel3 to SO2 (Left)</source>
        <translation>Canale 3 a SO2 (sinistra)</translation>
    </message>
    <message>
        <source>Channel4 to SO2 (Left)</source>
        <translation>Canale 4 a SO2 (sinistra)</translation>
    </message>
    <message>
        <source>Wave Pattern</source>
        <translation>Wave Pattern</translation>
    </message>
    <message>
        <source>The amount of increase or decrease in frequency</source>
        <translation>La quantità di aumento o diminuzione di frequenza</translation>
    </message>
    <message>
        <source>The rate at which increase or decrease in frequency occurs</source>
        <translation>La velocità a cui l&apos;aumento o la diminuzione di frequenza avvengono</translation>
    </message>
    <message>
        <source>The duty cycle is the ratio of the duration (time) that a signal is ON versus the total period of the signal.</source>
        <translation>Il duty cycle è il rapporto tra il tempo in cui il segnale è ON e il periodo completo del segnale.</translation>
    </message>
    <message>
        <source>Square Channel 1 Volume</source>
        <translation>Volume square del canale 1</translation>
    </message>
    <message>
        <source>The delay between step change</source>
        <translation>Ritardo tra i cambi di step</translation>
    </message>
    <message>
        <source>Draw the wave here</source>
        <translation>Disegnare l&apos;onda qui</translation>
    </message>
</context>
<context>
    <name>pattern</name>
    <message>
        <source>Cannot freeze pattern</source>
        <translation>Impossibile congelare il pattern</translation>
    </message>
    <message>
        <source>The pattern currently cannot be freezed because you&apos;re in play-mode. Please stop and try again!</source>
        <translation>Non è stato possibile congelare questo pattern perché in modalità riproduzione. Fermare la riproduzione e riprovare!</translation>
    </message>
</context>
<context>
    <name>patternFreezeStatusDialog</name>
    <message>
        <source>Freezing pattern...</source>
        <translation>Congelamento del pattern...</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation>Annulla</translation>
    </message>
</context>
<context>
    <name>patternView</name>
    <message>
        <source>double-click to open this pattern in piano-roll
use mouse wheel to set volume of a step</source>
        <translation>un doppio click apre questo pattern nel piano-roll
la rotellina del mouse impostare il volume delle note</translation>
    </message>
    <message>
        <source>Open in piano-roll</source>
        <translation>Apri nel piano-roll</translation>
    </message>
    <message>
        <source>Clear all notes</source>
        <translation>Cancella tutte le note</translation>
    </message>
    <message>
        <source>Reset name</source>
        <translation>Reimposta il nome</translation>
    </message>
    <message>
        <source>Change name</source>
        <translation>Cambia nome</translation>
    </message>
    <message>
        <source>Refreeze</source>
        <translation>Congela nuovamente</translation>
    </message>
    <message>
        <source>Freeze</source>
        <translation>Congela</translation>
    </message>
    <message>
        <source>Unfreeze</source>
        <translation>Scongela</translation>
    </message>
    <message>
        <source>Add steps</source>
        <translation>Aggiungi note</translation>
    </message>
    <message>
        <source>Remove steps</source>
        <translation>Elimina note</translation>
    </message>
</context>
<context>
    <name>PianoRoll</name>
    <message>
        <source>Play/pause current pattern (Space)</source>
        <translation>Riproduci/metti in pausa questo pattern (Spazio)</translation>
    </message>
    <message>
        <source>Record notes from MIDI-device/channel-piano</source>
        <translation>Registra note da una periferica/canale piano MIDI</translation>
    </message>
    <message>
        <source>Stop playing of current pattern (Space)</source>
        <translation>Ferma la riproduzione di questo pattern (Spazio)</translation>
    </message>
    <message>
        <source>Cut selected notes (Ctrl+X)</source>
        <translation>Taglia le note selezionate (Ctrl+X)</translation>
    </message>
    <message>
        <source>Copy selected notes (Ctrl+C)</source>
        <translation>Copia le note selezionate (Ctrl+C)</translation>
    </message>
    <message>
        <source>Paste notes from clipboard (Ctrl+V)</source>
        <translation>Incolla le note selezionate (Ctrl+V)</translation>
    </message>
    <message>
        <source>Piano-Roll - no pattern</source>
        <translation>Piano-Roll - nessun pattern</translation>
    </message>
    <message>
        <source>Piano-Roll - %1</source>
        <translation>Piano-Roll - %1</translation>
    </message>
    <message>
        <source>Please open a pattern by double-clicking on it!</source>
        <translation>Aprire un pattern con un doppio-click sul pattern stesso!</translation>
    </message>
    <message>
        <source>Last note</source>
        <translation>Ultima nota</translation>
    </message>
    <message>
        <source>Record notes from MIDI-device/channel-piano while playing song or BB track</source>
        <translation>Registra note da una periferica MIDI/canale piano mentre la traccia o la BB track è in riproduzione</translation>
    </message>
    <message>
        <source>Draw mode (Shift+D)</source>
        <translation>Modalità disegno (Shift+D)</translation>
    </message>
    <message>
        <source>Erase mode (Shift+E)</source>
        <translation>Modalità cancellazione (Shift+E)</translation>
    </message>
    <message>
        <source>Select mode (Shift+S)</source>
        <translation>Modalità selezione (Shift+S)</translation>
    </message>
    <message>
        <source>Click here to play the current pattern. This is useful while editing it. The pattern is automatically looped when its end is reached.</source>
        <translation>Cliccando qui si riproduce il pattern selezionato. Questo è utile mentre lo  si modifica. Il pattern viene automaticamente ripetuto quando finisce.</translation>
    </message>
    <message>
        <source>Click here to record notes from a MIDI-device or the virtual test-piano of the according channel-window to the current pattern. When recording all notes you play will be written to this pattern and you can play and edit them afterwards.</source>
        <translation>Cliccando qui si registrano nel pattern note da una periferica MIDI o dal piano di prova virtuale nella finestra del canale corrispondente. Mentre si registra, tutte le note eseguite vengono scritte in questo pattern e in seguito le si potrà riprodurre e modificare.</translation>
    </message>
    <message>
        <source>Click here to record notes from a MIDI-device or the virtual test-piano of the according channel-window to the current pattern. When recording all notes you play will be written to this pattern and you will hear the song or BB track in the background.</source>
        <translation>Cliccando qui si registrano nel pattern note da una periferica MIDI o dal piano di prova virtuale nella finestra del canale corrispondente. Mentre si registra, tutte le note eseguite vengono scritte in questo pattern, sentendo contemporaneamente la canzone o la traccia BB in sottofondo.</translation>
    </message>
    <message>
        <source>Click here to stop playback of current pattern.</source>
        <translation>Cliccando qui si ferma la riproduzione del pattern attivo.</translation>
    </message>
    <message>
        <source>Click here and the selected notes will be cut into the clipboard. You can paste them anywhere in any pattern by clicking on the paste button.</source>
        <translation>Cliccando qui le note selezionate verranno spostate negli appunti. È possibile incollarle in un punto qualsiasi del pattern cliccando sul tasto incolla.</translation>
    </message>
    <message>
        <source>Click here and the selected notes will be copied into the clipboard. You can paste them anywhere in any pattern by clicking on the paste button.</source>
        <translation>Cliccando qui le note selezionate verranno copiate negli appunti. È possibile incollarle in un punto qualsiasi del pattern cliccando sul tasto incolla.</translation>
    </message>
    <message>
        <source>Click here and the notes from the clipboard will be pasted at the first visible measure.</source>
        <translation>Cliccando qui i valori nella clipboard vengono incollati alla prima battuta visibile.</translation>
    </message>
    <message>
        <source>Note lock</source>
        <translation>Note lock</translation>
    </message>
    <message>
        <source>Note Volume</source>
        <translation>Volume Note</translation>
    </message>
    <message>
        <source>Note Panning</source>
        <translation>Panning Note</translation>
    </message>
    <message>
        <source>Detune mode (Shift+T)</source>
        <translation>Modalità intonanzione (Shift+T)</translation>
    </message>
    <message>
        <source>Click here and draw mode will be activated. In this mode you can add, resize and move notes. This is the default mode which is used most of the time. You can also press &apos;Shift+D&apos; on your keyboard to activate this mode. In this mode, hold Ctrl to temporarily go into select mode.</source>
        <translation>Cliccando qui si attiva la modalità disegno. In questa modalità è possibile aggiungere e spostare singoli valori. Questa è la modalità predefinita, che viene usata la maggior parte del tempo. Questa modalità si attiva anche premendo la combinazione di tasti &apos;Shift+D&apos;. Tieni premuto Ctfl per andare temporaneamente in modalità selezione.</translation>
    </message>
    <message>
        <source>Click here and erase mode will be activated. In this mode you can erase notes. You can also press &apos;Shift+E&apos; on your keyboard to activate this mode.</source>
        <translation>Cliccando qui si attiva la modalità cancellazione. In questa modalità è possibile cancellare singoli valori. Questa modalità si attiva anche premendo la combinazione di tasti &apos;Shift+E&apos;.</translation>
    </message>
    <message>
        <source>Click here and select mode will be activated. In this mode you can select notes. Alternatively, you can hold Ctrl in draw mode to temporarily use select mode.</source>
        <translation>Cliccando qui viene attivata la modalità selezione. Puoi selezionare le note. Puoi anche tenere premuto Ctrl durante la modalità disegno per usare la modalità selezione temporaneamente.</translation>
    </message>
    <message>
        <source>Click here and detune mode will be activated. In this mode you can click a note to open its automation detuning. You can utilize this to slide notes from one to another. You can also press &apos;Shift+T&apos; on your keyboard to activate this mode.</source>
        <translation>Cliccando qui viene attivata la modalità intonazione. Puoi cliccare una nota per aprire la finestra di automazione dell&apos;intonazione. Puoi usare questa modalità per fare uno slide da una nota ad un&apos;altra. Puoi anche premere Shift+T per attivare questa modalità.</translation>
    </message>
    <message>
        <source>Mark/unmark current semitone</source>
        <translation>Evidenza (o togli evidenziazione) questo semitono</translation>
    </message>
    <message>
        <source>Mark current scale</source>
        <translation>Evidenza la scala corrente</translation>
    </message>
    <message>
        <source>Mark current chord</source>
        <translation>Evidenza l&apos;accordo corrente</translation>
    </message>
    <message>
        <source>Unmark all</source>
        <translation>Togli tutte le evidenziazioni</translation>
    </message>
    <message>
        <source>No scale</source>
        <translation>- Scale</translation>
    </message>
    <message>
        <source>No chord</source>
        <translation>- Accordi</translation>
    </message>
</context>
<context>
    <name>pluginBrowser</name>
    <message>
        <source>Instrument plugins</source>
        <translation>Plugin strumentali</translation>
    </message>
    <message>
        <source>three powerful oscillators you can modulate in several ways</source>
        <translation>tre potenti oscillatori che puoi modulare in diversi modi</translation>
    </message>
    <message>
        <source>no description</source>
        <translation>nessuna descrizione</translation>
    </message>
    <message>
        <source>VST-host for using VST(i)-plugins within LMMS</source>
        <translation>Host VST per usare i plugin VST con LMMS</translation>
    </message>
    <message>
        <source>simple sampler with various settings for using samples (e.g. drums) in an instrument-track</source>
        <translation>Semplice campionatore con molte impostazioni per usare campioni (ad es. di percussioni) in una traccia</translation>
    </message>
    <message>
        <source>Filter for importing FL Studio projects into LMMS</source>
        <translation>Filtro per importare progetti di FL Studio in LMMS</translation>
    </message>
    <message>
        <source>Filter for importing MIDI-files into LMMS</source>
        <translation>Filtro per importare file MIDI in LMMS</translation>
    </message>
    <message>
        <source>Additive Synthesizer for organ-like sounds</source>
        <translation>Sintetizzatore additivo per suoni tipo organo</translation>
    </message>
    <message>
        <source>Vibrating string modeler</source>
        <translation>Modulatore di corde vibranti</translation>
    </message>
    <message>
        <source>Plugin for enhancing stereo separation of a stereo input file</source>
        <translation>Plugin per migliorare la separazione stereo di un file</translation>
    </message>
    <message>
        <source>Plugin for controlling knobs with sound peaks</source>
        <translation>Plugin per controllare le manopole con picchi di suono</translation>
    </message>
    <message>
        <source>versatile kick- &amp; bassdrum-synthesizer</source>
        <translation>sintetizzatore di colpo di cassa versatile</translation>
    </message>
    <message>
        <source>GUS-compatible patch instrument</source>
        <translation>strumento compatibile con GUS</translation>
    </message>
    <message>
        <source>plugin for using arbitrary LADSPA-effects inside LMMS.</source>
        <translation>Plugin per usare qualsiasi effetto LADSPA in LMMS.</translation>
    </message>
    <message>
        <source>Plugin for freely manipulating stereo output</source>
        <translation>Plugin per manipolare liberamente un&apos;uscita stereo</translation>
    </message>
    <message>
        <source>Incomplete monophonic imitation tb303</source>
        <translation>Imitazione monofonica del tb303 incompleta</translation>
    </message>
    <message>
        <source>plugin for using arbitrary VST-effects inside LMMS.</source>
        <translation>Plugin per usare qualsiasi effetto VST in LMMS.</translation>
    </message>
    <message>
        <source>Tuneful things to bang on</source>
        <translation>Oggetti dotati di intonazione su cui picchiare</translation>
    </message>
    <message>
        <source>plugin for boosting bass</source>
        <translation>Plugin per aumentare notevolmente i bassi</translation>
    </message>
    <message>
        <source>List installed LADSPA plugins</source>
        <translation>Elenca i plugin LADSPA installati</translation>
    </message>
    <message>
        <source>Instrument browser</source>
        <translation>Navigatore degli strumenti</translation>
    </message>
    <message>
        <source>Drag an instrument into either the Song-Editor, the Beat+Bassline Editor or into an existing instrument track.</source>
        <translation>È possibile trascinare uno strumento nel Song-Editor, nel Beat+Bassline Editor o direttamente in un canale esistente.</translation>
    </message>
    <message>
        <source>Emulation of the MOS6581 and MOS8580 SID.
This chip was used in the Commodore 64 computer.</source>
        <translation>Emulazione di MOS6581 and MOS8580 SID.
Questo chip era utilizzato nel Commode 64.</translation>
    </message>
    <message>
        <source>Player for SoundFont files</source>
        <translation>Riproduttore di file SounFont</translation>
    </message>
    <message>
        <source>Emulation of GameBoy (TM) APU</source>
        <translation>Emulatore di GameBoy™ APU</translation>
    </message>
    <message>
        <source>Customizable wavetable synthesizer</source>
        <translation>Sintetizzatore wavetable configurabile</translation>
    </message>
    <message>
        <source>Embedded ZynAddSubFX</source>
        <translation>ZynAddSubFX incorporato</translation>
    </message>
    <message>
        <source>2-operator FM Synth</source>
        <translation>Sintetizzatore FM a 2 operatori</translation>
    </message>
    <message>
        <source>Filter for importing Hydrogen files into LMMS</source>
        <translation>Strumento per l&apos;importazione di file Hydrogen dentro LMMS</translation>
    </message>
    <message>
        <source>LMMS port of sfxr</source>
        <translation>Port di sfxr su LMMS</translation>
    </message>
</context>
<context>
    <name>projectNotes</name>
    <message>
        <source>Project notes</source>
        <translation>Note del progetto</translation>
    </message>
    <message>
        <source>Put down your project notes here.</source>
        <translation>Scrivi qui le note per il tuo progetto.</translation>
    </message>
    <message>
        <source>Edit Actions</source>
        <translation>Modifica azioni</translation>
    </message>
    <message>
        <source>&amp;Undo</source>
        <translation>&amp;Annulla operazione</translation>
    </message>
    <message>
        <source>Ctrl+Z</source>
        <translation>Ctrl+Z</translation>
    </message>
    <message>
        <source>&amp;Redo</source>
        <translation>&amp;Ripeti operazione</translation>
    </message>
    <message>
        <source>Ctrl+Y</source>
        <translation>Ctrl+Y</translation>
    </message>
    <message>
        <source>&amp;Copy</source>
        <translation>&amp;Copia</translation>
    </message>
    <message>
        <source>Ctrl+C</source>
        <translation>Ctrl+C</translation>
    </message>
    <message>
        <source>Cu&amp;t</source>
        <translation>&amp;Taglia</translation>
    </message>
    <message>
        <source>Ctrl+X</source>
        <translation>Ctrl+X</translation>
    </message>
    <message>
        <source>&amp;Paste</source>
        <translation>&amp;Incolla</translation>
    </message>
    <message>
        <source>Ctrl+V</source>
        <translation>Ctrl+V</translation>
    </message>
    <message>
        <source>Format Actions</source>
        <translation>Formatta azioni</translation>
    </message>
    <message>
        <source>&amp;Bold</source>
        <translation>&amp;Grassetto</translation>
    </message>
    <message>
        <source>Ctrl+B</source>
        <translation>Ctrl+B</translation>
    </message>
    <message>
        <source>&amp;Italic</source>
        <translation>&amp;Corsivo</translation>
    </message>
    <message>
        <source>Ctrl+I</source>
        <translation>Ctrl+I</translation>
    </message>
    <message>
        <source>&amp;Underline</source>
        <translation>&amp;Sottolineato</translation>
    </message>
    <message>
        <source>Ctrl+U</source>
        <translation>Ctrl+U</translation>
    </message>
    <message>
        <source>&amp;Left</source>
        <translation>&amp;Sinistra</translation>
    </message>
    <message>
        <source>Ctrl+L</source>
        <translation>Ctrl+L</translation>
    </message>
    <message>
        <source>C&amp;enter</source>
        <translation>&amp;Centro</translation>
    </message>
    <message>
        <source>Ctrl+E</source>
        <translation>Ctrl+E</translation>
    </message>
    <message>
        <source>&amp;Right</source>
        <translation>&amp;Destra</translation>
    </message>
    <message>
        <source>Ctrl+R</source>
        <translation>Ctrl+R</translation>
    </message>
    <message>
        <source>&amp;Justify</source>
        <translation>&amp;Giustifica</translation>
    </message>
    <message>
        <source>Ctrl+J</source>
        <translation>Ctrl+J</translation>
    </message>
    <message>
        <source>&amp;Color...</source>
        <translation>&amp;Colore...</translation>
    </message>
</context>
<context>
    <name>renameDialog</name>
    <message>
        <source>Rename...</source>
        <translation>Rinomina...</translation>
    </message>
</context>
<context>
    <name>setupDialog</name>
    <message>
        <source>Setup LMMS</source>
        <translation>Cofigura LMMS</translation>
    </message>
    <message>
        <source>General settings</source>
        <translation>Impostazioni generali</translation>
    </message>
    <message>
        <source>BUFFER SIZE</source>
        <translation>DIMENSIONE DEL BUFFER</translation>
    </message>
    <message>
        <source>Reset to default-value</source>
        <translation>Reimposta al valore predefinito</translation>
    </message>
    <message>
        <source>MISC</source>
        <translation>VARIE</translation>
    </message>
    <message>
        <source>Audio settings</source>
        <translation>Impostazioni audio</translation>
    </message>
    <message>
        <source>AUDIO INTERFACE</source>
        <translation>INTERFACCIA AUDIO</translation>
    </message>
    <message>
        <source>MIDI settings</source>
        <translation>Impostazioni MIDI</translation>
    </message>
    <message>
        <source>MIDI INTERFACE</source>
        <translation>INTERFACCIA MIDI</translation>
    </message>
    <message>
        <source>OK</source>
        <translation>OK</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation>Annulla</translation>
    </message>
    <message>
        <source>Restart LMMS</source>
        <translation>Riavvia LMMS</translation>
    </message>
    <message>
        <source>Please note that most changes won&apos;t take effect until you restart LMMS!</source>
        <translation>Si prega di notare che la maggior parte delle modifiche non avrà effetto fino al riavvio di LMMS!</translation>
    </message>
    <message>
        <source>Here you can setup the internal buffer-size used by LMMS. Smaller values result in a lower latency but also may cause unusable sound or bad performance, especially on older computers or systems with a non-realtime kernel.</source>
        <translation>Qui è possibile impostare la dimensione del buffer interno usato da LMMS. Valori più piccoli danno come risultato una latenza più bassa ma possono causare una qualità audio inutilizzabile o cattive prestazioni, specialmente su computer datati o sistemi con kernel non-realtime.</translation>
    </message>
    <message>
        <source>Here you can select your preferred audio-interface. Depending on the configuration of your system during compilation time you can choose between ALSA, JACK, OSS and more. Below you see a box which offers controls to setup the selected audio-interface.</source>
        <translation>Qui è possibile selezionare l&apos;interfaccia audio. A seconda della configurazione del tuo sistema in fase di compilazione puoi scegliere tra ALSA, JACK, OSS e altri. Sotto trovi una casella che offre dei controlli per l&apos;interfaccia audio selezionata.</translation>
    </message>
    <message>
        <source>Here you can select your preferred MIDI-interface. Depending on the configuration of your system during compilation time you can choose between ALSA, OSS and more. Below you see a box which offers controls to setup the selected MIDI-interface.</source>
        <translation>Qui è possibile selezionare l&apos;interfaccia MIDI. A seconda della configurazione del tuo sistema in fase di compilazione puoi scegliere tra ALSA, OSS e altri. Sotto si trova una casella che offre dei controlli per l&apos;interfaccia MIDI selezionata.</translation>
    </message>
    <message>
        <source>Display volume as dBV </source>
        <translation>Mostra il volume in dBV </translation>
    </message>
    <message>
        <source>LMMS working directory</source>
        <translation>Directory di lavoro di LMMS</translation>
    </message>
    <message>
        <source>VST-plugin directory</source>
        <translation>Directory dei plugin VST</translation>
    </message>
    <message>
        <source>Artwork directory</source>
        <translation>DIrectory del tema grafico</translation>
    </message>
    <message>
        <source>FL Studio installation directory</source>
        <translation>Directory di installazione di FL Studio</translation>
    </message>
    <message>
        <source>Performance settings</source>
        <translation>Impostazioni prestazioni</translation>
    </message>
    <message>
        <source>UI effects vs. performance</source>
        <translation>Effetti UI (interfaccia grafica) vs. prestazioni</translation>
    </message>
    <message>
        <source>Frames: %1
Latency: %2 ms</source>
        <translation>Frames: %1
Latenza: %2 ms</translation>
    </message>
    <message>
        <source>Choose LMMS working directory</source>
        <translation>Seleziona la directory di lavoro di LMMS</translation>
    </message>
    <message>
        <source>Choose your VST-plugin directory</source>
        <translation>Seleziona la tua directory dei plugin VST</translation>
    </message>
    <message>
        <source>Choose artwork-theme directory</source>
        <translation>Seleziona la directory del tema grafico</translation>
    </message>
    <message>
        <source>Choose FL Studio installation directory</source>
        <translation>Seleziona la directory di installazione di FL Studio</translation>
    </message>
    <message>
        <source>Enable tooltips</source>
        <translation>Abilita i suggerimenti</translation>
    </message>
    <message>
        <source>Show restart warning after changing settings</source>
        <translation>Dopo aver modificato le impostazioni, mostra un avviso al riavvio</translation>
    </message>
    <message>
        <source>Compress project files per default</source>
        <translation>Per impostazione predefinita, comprimi i file di progetto</translation>
    </message>
    <message>
        <source>HQ-mode for output audio-device</source>
        <translation>Modalità alta qualità per l&apos;uscita audio</translation>
    </message>
    <message>
        <source>STK rawwave directory</source>
        <translation>Directory per i file rawwave STK</translation>
    </message>
    <message>
        <source>Choose LADSPA plugin directory</source>
        <translation>Seleziona le directory dei plugin LADSPA</translation>
    </message>
    <message>
        <source>Choose STK rawwave directory</source>
        <translation>Seleziona le directory dei file rawwave STK</translation>
    </message>
    <message>
        <source>Paths</source>
        <translation>Percorsi</translation>
    </message>
    <message>
        <source>LADSPA plugin paths</source>
        <translation>Percorsi dei plugin LADSPA</translation>
    </message>
    <message>
        <source>Default Soundfont File</source>
        <translation>File SoundFont predefinito</translation>
    </message>
    <message>
        <source>Background artwork</source>
        <translation>Grafica dello sfondo</translation>
    </message>
    <message>
        <source>Choose default SoundFont</source>
        <translation>Scegliere il SoundFont predefinito</translation>
    </message>
    <message>
        <source>Choose background artwork</source>
        <translation>Scegliere la grafica dello sfondo</translation>
    </message>
    <message>
        <source>One instrument track window mode</source>
        <translation>Modalità finestra ad una traccia strumento</translation>
    </message>
    <message>
        <source>Compact track buttons</source>
        <translation>Pulsanti della traccia compatti</translation>
    </message>
    <message>
        <source>Sync VST plugins to host playback</source>
        <translation>Sincronizza i plugin VST al playback dell&apos;host</translation>
    </message>
    <message>
        <source>Enable note labels in piano roll</source>
        <translation>Abilita i nomi delle note nel piano-roll</translation>
    </message>
    <message>
        <source>Enable waveform display by default</source>
        <translation>Abilità il display della forma d&apos;onda per default</translation>
    </message>
    <message>
        <source>Smooth scroll in Song Editor</source>
        <translation>Scorrimento morbido nel Song-Editor</translation>
    </message>
    <message>
        <source>Enable auto save feature</source>
        <translation>Abilita la funzione di salvataggio automatico</translation>
    </message>
    <message>
        <source>Show playback cursor in AudioFileProcessor</source>
        <translation>Mostra il cursore di riproduzione dentro AudioFileProcessor</translation>
    </message>
</context>
<context>
    <name>sf2Instrument</name>
    <message>
        <source>Bank</source>
        <translation>Banco</translation>
    </message>
    <message>
        <source>Gain</source>
        <translation>Guadagno</translation>
    </message>
    <message>
        <source>Reverb</source>
        <translation>Riverbero</translation>
    </message>
    <message>
        <source>Reverb Roomsize</source>
        <translation>Riverbero - dimensione stanza</translation>
    </message>
    <message>
        <source>Reverb Damping</source>
        <translation>Riverbero - attenuazione</translation>
    </message>
    <message>
        <source>Reverb Width</source>
        <translation>Riverbero - ampiezza</translation>
    </message>
    <message>
        <source>Reverb Level</source>
        <translation>Riverbero - livello</translation>
    </message>
    <message>
        <source>Chorus Lines</source>
        <translation>Chorus - voci</translation>
    </message>
    <message>
        <source>Chorus Level</source>
        <translation>Chorus - livello</translation>
    </message>
    <message>
        <source>Chorus Speed</source>
        <translation>Chorus - velocità</translation>
    </message>
    <message>
        <source>Chorus Depth</source>
        <translation>Chorus - profondità</translation>
    </message>
    <message>
        <source>Patch</source>
        <translation>Patch</translation>
    </message>
    <message>
        <source>Chorus</source>
        <translation>Chorus</translation>
    </message>
</context>
<context>
    <name>sf2InstrumentView</name>
    <message>
        <source>Open other SoundFont file</source>
        <translation>Apri un altro file SoundFont</translation>
    </message>
    <message>
        <source>Click here to open another SF2 file</source>
        <translation>Clicca qui per aprire un altro file SF2</translation>
    </message>
    <message>
        <source>Choose the patch</source>
        <translation>Seleziona il patch</translation>
    </message>
    <message>
        <source>Gain</source>
        <translation>Guadagno</translation>
    </message>
    <message>
        <source>Apply reverb (if supported)</source>
        <translation>Applica il riverbero (se supportato)</translation>
    </message>
    <message>
        <source>This button enables the reverb effect. This is useful for cool effects, but only works on files that support it.</source>
        <translation>Questo pulsante abilita l&apos;effetto riverbero, che è utile per effetti particolari ma funziona solo su file che lo supportano.</translation>
    </message>
    <message>
        <source>Reverb Roomsize:</source>
        <translation>Riverbero - dimensione stanza:</translation>
    </message>
    <message>
        <source>Reverb Damping:</source>
        <translation>Riverbero - attenuazione:</translation>
    </message>
    <message>
        <source>Reverb Width:</source>
        <translation>Riverbero - ampiezza:</translation>
    </message>
    <message>
        <source>Reverb Level:</source>
        <translation>Riverbero - livello:</translation>
    </message>
    <message>
        <source>Apply chorus (if supported)</source>
        <translation>Applica il chorus (se supportato)</translation>
    </message>
    <message>
        <source>This button enables the chorus effect. This is useful for cool echo effects, but only works on files that support it.</source>
        <translation>Questo pulsante abilita l&apos;effetto chorus, che è utile per effetti di eco particolari ma funziona solo su file che lo supportano.</translation>
    </message>
    <message>
        <source>Chorus Lines:</source>
        <translation>Chorus - voci:</translation>
    </message>
    <message>
        <source>Chorus Level:</source>
        <translation>Chorus - livello:</translation>
    </message>
    <message>
        <source>Chorus Speed:</source>
        <translation>Chorus - velocità:</translation>
    </message>
    <message>
        <source>Chorus Depth:</source>
        <translation>Chorus - profondità:</translation>
    </message>
    <message>
        <source>Open SoundFont file</source>
        <translation>Apri un file SoundFont</translation>
    </message>
    <message>
        <source>SoundFont2 Files (*.sf2)</source>
        <translation>File SoundFont2 (*.sf2)</translation>
    </message>
</context>
<context>
    <name>sfxrInstrument</name>
    <message>
        <source>Wave Form</source>
        <translation>Forma d&apos;onda</translation>
    </message>
</context>
<context>
    <name>sidInstrument</name>
    <message>
        <source>Cutoff</source>
        <translation>Taglio</translation>
    </message>
    <message>
        <source>Resonance</source>
        <translation>Risonanza</translation>
    </message>
    <message>
        <source>Filter type</source>
        <translation>Tipo di filtro</translation>
    </message>
    <message>
        <source>Voice 3 off</source>
        <translation>Voce 3 spenta</translation>
    </message>
    <message>
        <source>Volume</source>
        <translation>Volume</translation>
    </message>
    <message>
        <source>Chip model</source>
        <translation>Modello di chip</translation>
    </message>
</context>
<context>
    <name>sidInstrumentView</name>
    <message>
        <source>Volume:</source>
        <translation>Volume:</translation>
    </message>
    <message>
        <source>Resonance:</source>
        <translation>Risonanza:</translation>
    </message>
    <message>
        <source>Cutoff frequency:</source>
        <translation>Frequenza di taglio:</translation>
    </message>
    <message>
        <source>High-Pass filter </source>
        <translation>Filtro passa-alto</translation>
    </message>
    <message>
        <source>Band-Pass filter </source>
        <translation>Filtro passa-banda</translation>
    </message>
    <message>
        <source>Low-Pass filter </source>
        <translation>Filtro passa-basso</translation>
    </message>
    <message>
        <source>Voice3 Off </source>
        <translation>Voce 3 spenta</translation>
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
        <translation>Attacco:</translation>
    </message>
    <message>
        <source>Decay:</source>
        <translation>Decadimento:</translation>
    </message>
    <message>
        <source>Sustain:</source>
        <translation>Sostegno:</translation>
    </message>
    <message>
        <source>Release:</source>
        <translation>Rilascio:</translation>
    </message>
    <message>
        <source>Pulse Width:</source>
        <translation>Ampiezza pulse:</translation>
    </message>
    <message>
        <source>Coarse:</source>
        <translation>Approssimativo:</translation>
    </message>
    <message>
        <source>Pulse Wave</source>
        <translation>Onda pulse</translation>
    </message>
    <message>
        <source>Triangle Wave</source>
        <translation>Onda triangolare</translation>
    </message>
    <message>
        <source>SawTooth</source>
        <translation>Dente di sega</translation>
    </message>
    <message>
        <source>Noise</source>
        <translation>Rumore</translation>
    </message>
    <message>
        <source>Sync</source>
        <translation>Sincronizzato</translation>
    </message>
    <message>
        <source>Ring-Mod</source>
        <translation>Modulazione ring</translation>
    </message>
    <message>
        <source>Filtered</source>
        <translation>Filtrato</translation>
    </message>
    <message>
        <source>Attack rate determines how rapidly the output of Voice %1 rises from zero to peak amplitude.</source>
        <translation>Il livello di attacco determina quanto rapidamente l&apos;uscita della voce %1 sale da zero al picco di amplificazione.</translation>
    </message>
    <message>
        <source>Decay rate determines how rapidly the output falls from the peak amplitude to the selected Sustain level.</source>
        <translation>Il livello di decadimento determina quanto rapidamente l&apos;uscita ricade dal picco di amplificazione al livello di sostegno impostato.</translation>
    </message>
    <message>
        <source>Output of Voice %1 will remain at the selected Sustain amplitude as long as the note is held.</source>
        <translation>L&apos;uscita della voce %1 rimarrà al livello di sostegno impostato per tutta la durata della nota.</translation>
    </message>
    <message>
        <source>The output of of Voice %1 will fall from Sustain amplitude to zero amplitude at the selected Release rate.</source>
        <translation>L&apos;uscita della voce %1 ricadrà dal livello di sostegno verso il silenzio con la velocità di rilascio impostata.</translation>
    </message>
    <message>
        <source>The Pulse Width resolution allows the width to be smoothly swept with no discernable stepping. The Pulse waveform on Oscillator %1 must be selected to have any audible effect.</source>
        <translation>La risoluzione dell&apos;ampiezza del Pulse permette di impostare che l&apos;ampiezza  venga variata in modo continuo senza salti udibili. Sull&apos;oscillatore %1 deve essere selezionata la forma d&apos;onda Pulse perché si abbia un effetto udibile.</translation>
    </message>
    <message>
        <source>The Coarse detuning allows to detune Voice %1 one octave up or down.</source>
        <translation>L&apos;intonazione permette di &quot;stonare&quot; la voce %1 verso l&apos;alto o verso il basso di un&apos;ottava.</translation>
    </message>
    <message>
        <source>Sync synchronizes the fundamental frequency of Oscillator %1 with the fundamental frequency of Oscillator %2 producing &quot;Hard Sync&quot; effects.</source>
        <translation>Il Sync sincronizza la frequenza fondamentale dell&apos;oscillatore %1 con quella dell&apos;oscillatore %2 producendo effetti di &quot;Hard Sync&quot;.</translation>
    </message>
    <message>
        <source>Ring-mod replaces the Triangle Waveform output of Oscillator %1 with a &quot;Ring Modulated&quot; combination of Oscillators %1 and %2.</source>
        <translation>Ring-mod rimpiazza l&apos;uscita della forma d&apos;onda triangolare dell&apos;oscillatore %1 con la combinazione &quot;Ring Modulated&quot; degli oscillatori %1 e %2.</translation>
    </message>
    <message>
        <source>When Filtered is on, Voice %1 will be processed through the Filter. When Filtered is off, Voice %1 appears directly at the output, and the Filter has no effect on it.</source>
        <translation>Quando il filtraggio è attivo, la voce %1 verrà processata dal filtro. Quando il filtraggio è spento, la voce %1 arriva direttamente all&apos;uscita e il filtro non ha effetto su di essa.</translation>
    </message>
    <message>
        <source>Test</source>
        <translation>Test</translation>
    </message>
    <message>
        <source>Test, when set, resets and locks Oscillator %1 at zero until Test is turned off.</source>
        <translation>Quando Test è attivo, e finché non viene spento, reimposta e blocca l&apos;oscillatore %1 a zero.</translation>
    </message>
</context>
<context>
    <name>song</name>
    <message>
        <source>Project saved</source>
        <translation>Progeto salvato</translation>
    </message>
    <message>
        <source>The project %1 is now saved.</source>
        <translation>Il progetto %1 è stato salvato.</translation>
    </message>
    <message>
        <source>Project NOT saved.</source>
        <translation>Il progetto NON è stato salvato.</translation>
    </message>
    <message>
        <source>The project %1 was not saved!</source>
        <translation>Il progetto %1 non è stato salvato!</translation>
    </message>
    <message>
        <source>Import file</source>
        <translation>Importa file</translation>
    </message>
    <message>
        <source>untitled</source>
        <translation>senza_nome</translation>
    </message>
    <message>
        <source>Select file for project-export...</source>
        <translation>Scegliere il file per l&apos;esportazione del progetto...</translation>
    </message>
    <message>
        <source>Tempo</source>
        <translation>Tempo</translation>
    </message>
    <message>
        <source>Master volume</source>
        <translation>Volume principale</translation>
    </message>
    <message>
        <source>Master pitch</source>
        <translation>Altezza principale</translation>
    </message>
    <message>
        <source>Empty project</source>
        <translation>Empty project</translation>
    </message>
    <message>
        <source>This project is empty so exporting makes no sense. Please put some items into Song Editor first!</source>
        <translation>Questo progetto è vuoto, pertanto non c&apos;è nulla da esportare. Prima di esportare è necessario inserire alcuni elementi nel Song Editor!</translation>
    </message>
    <message>
        <source>MIDI sequences</source>
        <translation>Sequenze MIDI</translation>
    </message>
    <message>
        <source>FL Studio projects</source>
        <translation>Progetti FL Studio</translation>
    </message>
    <message>
        <source>All file types</source>
        <translation>Tutti i tipi di file</translation>
    </message>
    <message>
        <source>Hydrogen projects</source>
        <translation>Progetti Hydrogen</translation>
    </message>
    <message>
        <source>Select directory for writing exported tracks...</source>
        <translation>Seleziona una directory per le tracce esportate...</translation>
    </message>
</context>
<context>
    <name>SongEditor</name>
    <message>
        <source>Song-Editor</source>
        <translation>Song-Editor</translation>
    </message>
    <message>
        <source>Play song (Space)</source>
        <translation>Riproduci la canzone (Spazio)</translation>
    </message>
    <message>
        <source>Stop song (Space)</source>
        <translation>Ferma la riproduzione della canzone (Spazio)</translation>
    </message>
    <message>
        <source>Add beat/bassline</source>
        <translation>Aggiungi beat/bassline</translation>
    </message>
    <message>
        <source>Add sample-track</source>
        <translation>Aggiungi traccia di campione</translation>
    </message>
    <message>
        <source>Click here, if you want to play your whole song. Playing will be started at the song-position-marker (green). You can also move it while playing.</source>
        <translation>Cliccando qui si riproduce l&apos;intera canzone. La riproduzione inizierà alla posizione attuale del segnaposto (verde). È possibile spostarlo anche durante la riproduzione.</translation>
    </message>
    <message>
        <source>Click here, if you want to stop playing of your song. The song-position-marker will be set to the start of your song.</source>
        <translation>Cliccando qui si ferma la riproduzione della canzone. Il segnaposto verrà portato all&apos;inizio della canzone.</translation>
    </message>
    <message>
        <source>Could not open file</source>
        <translation>Non è stato possibile aprire il file</translation>
    </message>
    <message>
        <source>Could not write file</source>
        <translation>Impossibile scrivere il file</translation>
    </message>
    <message>
        <source>Draw mode</source>
        <translation>Modalità disegno</translation>
    </message>
    <message>
        <source>Edit mode (select and move)</source>
        <translation>Modalità modifica (seleziona e sposta)</translation>
    </message>
    <message>
        <source>Add automation-track</source>
        <translation>Aggiungi una traccia di automazione</translation>
    </message>
    <message>
        <source>Record samples from Audio-device</source>
        <translation>Registra campioni da una periferica audio</translation>
    </message>
    <message>
        <source>Record samples from Audio-device while playing song or BB track</source>
        <translation>Registra campioni da una periferica audio mentre la canzone o la BB track sono in riproduzione</translation>
    </message>
    <message>
        <source>Could not open file %1. You probably have no permissions to read this file.
 Please make sure to have at least read permissions to the file and try again.</source>
        <translation>Impossibile aprire il file %1. Probabilmente non disponi dei permessi necessari alla sua lettura.
Assicurati di avere almeno i permessi di lettura del file e prova di nuovo.</translation>
    </message>
    <message>
        <source>Error in file</source>
        <translation>Errore nel file</translation>
    </message>
    <message>
        <source>The file %1 seems to contain errors and therefore can&apos;t be loaded.</source>
        <translation>Il file %1 sembra contenere errori, quindi non può essere caricato.</translation>
    </message>
    <message>
        <source>Tempo</source>
        <translation>Tempo</translation>
    </message>
    <message>
        <source>TEMPO/BPM</source>
        <translation>TEMPO/BPM</translation>
    </message>
    <message>
        <source>tempo of song</source>
        <translation>tempo della canzone</translation>
    </message>
    <message>
        <source>The tempo of a song is specified in beats per minute (BPM). If you want to change the tempo of your song, change this value. Every measure has four beats, so the tempo in BPM specifies, how many measures / 4 should be played within a minute (or how many measures should be played within four minutes).</source>
        <translation>Il tempo della canzone è specificato in battiti al minuto (BPM). Per cambiare il tempo della canzone bisogna cambiare questo valore. Ogni marcatore ha 4 battiti, pertanto il tempo in BPM specifica quanti marcatori / 4 verranno riprodotti in un minuto (o quanti marcatori in 4 minuti).</translation>
    </message>
    <message>
        <source>High quality mode</source>
        <translation>Modalità ad alta qualità</translation>
    </message>
    <message>
        <source>Master volume</source>
        <translation>Volume principale</translation>
    </message>
    <message>
        <source>master volume</source>
        <translation>volume principale</translation>
    </message>
    <message>
        <source>Master pitch</source>
        <translation>Altezza principale</translation>
    </message>
    <message>
        <source>master pitch</source>
        <translation>altezza principale</translation>
    </message>
    <message>
        <source>Value: %1%</source>
        <translation>Valore: %1%</translation>
    </message>
    <message>
        <source>Value: %1 semitones</source>
        <translation>Valore: %1 semitoni</translation>
    </message>
    <message>
        <source>Could not open %1 for writing. You probably are not permitted to write to this file. Please make sure you have write-access to the file and try again.</source>
        <translation>Impossibile aprire il file %1 per la scrittura. Probabilmente non disponi dei permessi necessari alla scrittura di questo file. Assicurati di avere tali permessi e prova di nuovo.</translation>
    </message>
</context>
<context>
    <name>spectrumAnalyzerControlDialog</name>
    <message>
        <source>Linear spectrum</source>
        <translation>Spettro lineare</translation>
    </message>
    <message>
        <source>Linear Y axis</source>
        <translation>Asse Y lineare</translation>
    </message>
</context>
<context>
    <name>spectrumAnalyzerControls</name>
    <message>
        <source>Linear spectrum</source>
        <translation>Spettro lineare</translation>
    </message>
    <message>
        <source>Linear Y-axis</source>
        <translation>Asse Y lineare</translation>
    </message>
    <message>
        <source>Channel mode</source>
        <translation>Modalità del canale</translation>
    </message>
</context>
<context>
    <name>stereoEnhancerControlDialog</name>
    <message>
        <source>WIDE</source>
        <translation>AMPIO</translation>
    </message>
    <message>
        <source>Width:</source>
        <translation>Ampiezza:</translation>
    </message>
</context>
<context>
    <name>stereoEnhancerControls</name>
    <message>
        <source>Width</source>
        <translation>Ampiezza</translation>
    </message>
</context>
<context>
    <name>stereoMatrixControlDialog</name>
    <message>
        <source>Left to Left Vol:</source>
        <translation>Volume da Sinistra a Sinistra:</translation>
    </message>
    <message>
        <source>Left to Right Vol:</source>
        <translation>Volume da Sinistra a Destra:</translation>
    </message>
    <message>
        <source>Right to Left Vol:</source>
        <translation>Volume da Destra a Sinistra:</translation>
    </message>
    <message>
        <source>Right to Right Vol:</source>
        <translation>Volume da Destra a Destra:</translation>
    </message>
</context>
<context>
    <name>stereoMatrixControls</name>
    <message>
        <source>Left to Left</source>
        <translation>Da Sinistra a Sinistra</translation>
    </message>
    <message>
        <source>Left to Right</source>
        <translation>Da Sinistra a Destra</translation>
    </message>
    <message>
        <source>Right to Left</source>
        <translation>Da Destra a Sinistra</translation>
    </message>
    <message>
        <source>Right to Right</source>
        <translation>Da Destra a Destra</translation>
    </message>
</context>
<context>
    <name>timeLine</name>
    <message>
        <source>Enable/disable auto-scrolling</source>
        <translation>Abilita/disabilita lo scorrimento automatico</translation>
    </message>
    <message>
        <source>Enable/disable loop-points</source>
        <translation>Abilita/disabilita i punti di ripetizione</translation>
    </message>
    <message>
        <source>After stopping go back to begin</source>
        <translation>Una volta fermata la riproduzione, torna all&apos;inizio</translation>
    </message>
    <message>
        <source>After stopping go back to position at which playing was started</source>
        <translation>Una volta fermata la riproduzione, torna alla posizione da cui si è partiti</translation>
    </message>
    <message>
        <source>After stopping keep position</source>
        <translation>Una volta fermata la riproduzione, mantieni la posizione</translation>
    </message>
    <message>
        <source>Hint</source>
        <translation>Suggerimento</translation>
    </message>
    <message>
        <source>Press &lt;Ctrl&gt; to disable magnetic loop points.</source>
        <translation>Premi &lt;Ctrl&gt; per disabilitare i punti di loop magnetici.</translation>
    </message>
    <message>
        <source>Hold &lt;Shift&gt; to move the begin loop point; Press &lt;Ctrl&gt; to disable magnetic loop points.</source>
        <translation>Tieni premuto &lt;Shift&gt; per spostare l&apos;inizio del punto di loop; premi &lt;Ctrl&gt; per disabilitare i punti di loop magnetici.</translation>
    </message>
</context>
<context>
    <name>track</name>
    <message>
        <source>Muted</source>
        <translation>Muto</translation>
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
        <translation>Muto</translation>
    </message>
</context>
<context>
    <name>trackContentObjectView</name>
    <message>
        <source>Current position</source>
        <translation>Posizione attuale</translation>
    </message>
    <message>
        <source>Hint</source>
        <translation>Suggerimento</translation>
    </message>
    <message>
        <source>Press &lt;Ctrl&gt; and drag to make a copy.</source>
        <translation>Premere &lt;Ctrl&gt;, cliccare e trascinare per copiare.</translation>
    </message>
    <message>
        <source>Current length</source>
        <translation>Lunghezza attuale</translation>
    </message>
    <message>
        <source>Press &lt;Ctrl&gt; for free resizing.</source>
        <translation>Premere &lt;Ctrl&gt; per ridimensionare liberamente.</translation>
    </message>
    <message>
        <source>%1:%2 (%3:%4 to %5:%6)</source>
        <translation>%1:%2 (da %3:%4 a %5:%6)</translation>
    </message>
    <message>
        <source>Delete (middle mousebutton)</source>
        <translation>Elimina (tasto centrale del mouse)</translation>
    </message>
    <message>
        <source>Cut</source>
        <translation>Taglia</translation>
    </message>
    <message>
        <source>Copy</source>
        <translation>Copia</translation>
    </message>
    <message>
        <source>Paste</source>
        <translation>Incolla</translation>
    </message>
    <message>
        <source>Mute/unmute (&lt;Ctrl&gt; + middle click)</source>
        <translation>Attiva/disattiva la modalità muta (&lt;Ctrl&gt; + tasto centrale)</translation>
    </message>
</context>
<context>
    <name>trackOperationsWidget</name>
    <message>
        <source>Clone this track</source>
        <translation>Clona questa traccia</translation>
    </message>
    <message>
        <source>Remove this track</source>
        <translation>Elimina questa traccia</translation>
    </message>
    <message>
        <source>Press &lt;Ctrl&gt; while clicking on move-grip to begin a new drag&apos;n&apos;drop-action.</source>
        <translation>Premere &lt;Ctrl&gt; mentre si clicca sulla maniglia per lo spostamento per iniziare una nuova azione di drag&apos;n&apos;drop.</translation>
    </message>
    <message>
        <source>Actions for this track</source>
        <translation>Azioni per questa traccia</translation>
    </message>
    <message>
        <source>Mute</source>
        <translation>Muto</translation>
    </message>
    <message>
        <source>Mute this track</source>
        <translation>Metti questa traccia in modalità muta</translation>
    </message>
    <message>
        <source>Solo</source>
        <translation>Solo</translation>
    </message>
</context>
<context>
    <name>vestigeInstrument</name>
    <message>
        <source>Loading plugin</source>
        <translation>Caricamento plugin</translation>
    </message>
    <message>
        <source>Please wait while loading VST-plugin...</source>
        <translation>Prego attendere, caricamento del plugin VST...</translation>
    </message>
    <message>
        <source>Failed loading VST-plugin</source>
        <translation>Errore nel caricamento del plugin VST</translation>
    </message>
    <message>
        <source>The VST-plugin %1 could not be loaded for some reason.
If it runs with other VST-software under Linux, please contact an LMMS-developer!</source>
        <translation>Non è stato possibile caricare il plugin VST %1 a causa di alcuni errori.
Se, con altre applicazioni GNU/Linux il plugin funziona, si prega di contattare uno sviluppatore di LMMS!</translation>
    </message>
</context>
<context>
    <name>vibed</name>
    <message>
        <source>String %1 volume</source>
        <translation>Volume della corda %1</translation>
    </message>
    <message>
        <source>String %1 stiffness</source>
        <translation>Durezza della corda %1</translation>
    </message>
    <message>
        <source>Pick %1 position</source>
        <translation>Posizione del plettro %1</translation>
    </message>
    <message>
        <source>Pickup %1 position</source>
        <translation>Posizione del pickup %1</translation>
    </message>
    <message>
        <source>Detune %1</source>
        <translation>Intonazione %1</translation>
    </message>
    <message>
        <source>Length %1</source>
        <translation>Lunghezza %1</translation>
    </message>
    <message>
        <source>Impulse %1</source>
        <translation>Impulso %1</translation>
    </message>
    <message>
        <source>Octave %1</source>
        <translation>Ottava %1</translation>
    </message>
    <message>
        <source>Pan %1</source>
        <translation>Pan %1</translation>
    </message>
    <message>
        <source>Fuzziness %1 </source>
        <translation>Fuzziness %1</translation>
    </message>
</context>
<context>
    <name>vibedView</name>
    <message>
        <source>Volume:</source>
        <translation>Volume:</translation>
    </message>
    <message>
        <source>The &apos;V&apos; knob sets the volume of the selected string.</source>
        <translation>La manopola &apos;V&apos; regola il volume della corda selezionata.</translation>
    </message>
    <message>
        <source>String stiffness:</source>
        <translation>Durezza della corda:</translation>
    </message>
    <message>
        <source>The &apos;S&apos; knob sets the stiffness of the selected string.  The stiffness of the string affects how long the string will ring out.  The lower the setting, the longer the string will ring.</source>
        <translation>La manopola &apos;S&apos; regola la durezza della corda selezionata.  La durezza della corda influenza la durata della vibrazione. Più basso sarà il valore, più a lungo suonerà la corda.</translation>
    </message>
    <message>
        <source>Pick position:</source>
        <translation>Posizione del plettro:</translation>
    </message>
    <message>
        <source>The &apos;P&apos; knob sets the position where the selected string will be &apos;picked&apos;.  The lower the setting the closer the pick is to the bridge.</source>
        <translation>La manopola &apos;P&apos; regola il punto in cui verrà &apos;pizzicata&apos; la corda. Più basso sarà il valore, più vicino al ponte sarà il plettro.</translation>
    </message>
    <message>
        <source>Pickup position:</source>
        <translation>Posizione del pickup:</translation>
    </message>
    <message>
        <source>The &apos;PU&apos; knob sets the position where the vibrations will be monitored for the selected string.  The lower the setting, the closer the pickup is to the bridge.</source>
        <translation>La manopola &apos;PU&apos; regola la posizione in cui verranno rilevate le vibrazioni della corda selezionata. Più basso sarà il valore, più vicino al ponte sarà il pickup.</translation>
    </message>
    <message>
        <source>Pan:</source>
        <translation>Pan:</translation>
    </message>
    <message>
        <source>The Pan knob determines the location of the selected string in the stereo field.</source>
        <translation>La manopola Pan determina la posizione della corda selezionata nello spettro stereo.</translation>
    </message>
    <message>
        <source>Detune:</source>
        <translation>Intonazione:</translation>
    </message>
    <message>
        <source>The Detune knob modifies the pitch of the selected string.  Settings less than zero will cause the string to sound flat.  Settings greater than zero will cause the string to sound sharp.</source>
        <translation>La manopola Intonazione regola l&apos;altezza del suono della corda selezionata. Valori sotto lo zero sposteranno l&apos;intonazione della corda verso il bemolle. Valori sopra lo zero spostarenno l&apos;intonazione della corda verso il diesis.</translation>
    </message>
    <message>
        <source>Fuzziness:</source>
        <translation>Fuzziness:</translation>
    </message>
    <message>
        <source>The Slap knob adds a bit of fuzz to the selected string which is most apparent during the attack, though it can also be used to make the string sound more &apos;metallic&apos;.</source>
        <translation>La manopola Slap aggiungeun po&apos; di &quot;sporco&quot; al suono della corda selezionata, sensibile soprattutto nell&apos;attacco, anche se può essere usato per rendere il suono più &quot;metallico&quot;.</translation>
    </message>
    <message>
        <source>Length:</source>
        <translation>Lunghezza:</translation>
    </message>
    <message>
        <source>The Length knob sets the length of the selected string.  Longer strings will both ring longer and sound brighter, however, they will also eat up more CPU cycles.</source>
        <translation>La manopola Lunghezza regola la lunghezza della corda selezionata. Corde più lunghe suonano più a lungo e hanno un suono più brillante. Tuttavia richiedono anche più tempo del processore.</translation>
    </message>
    <message>
        <source>Impulse or initial state</source>
        <translation>Impulso o stato iniziale</translation>
    </message>
    <message>
        <source>The &apos;Imp&apos; selector determines whether the waveform in the graph is to be treated as an impulse imparted to the string by the pick or the initial state of the string.</source>
        <translation>Il selettore &apos;Imp&apos; determina se la forma d&apos;onda nel grafico deve essere trattata come l&apos;impulso del plettro sulla corda o come lo stato iniziale della corda.</translation>
    </message>
    <message>
        <source>Octave</source>
        <translation>Ottava</translation>
    </message>
    <message>
        <source>The Octave selector is used to choose which harmonic of the note the string will ring at.  For example, &apos;-2&apos; means the string will ring two octaves below the fundamental, &apos;F&apos; means the string will ring at the fundamental, and &apos;6&apos; means the string will ring six octaves above the fundamental.</source>
        <translation>Il seletore Ottava serve per scegliere a quale armonico della nota risuona la corda. Per esempio, &apos;-2&apos; significa che la corda risuona due ottave sotto la fondamentale, &apos;F&apos; significa che la corda risuona alla fondamentale e &apos;6&apos; significa che la corda risuona sei ottave sopra la fondamentale.</translation>
    </message>
    <message>
        <source>Impulse Editor</source>
        <translation>Editor dell&apos;impulso</translation>
    </message>
    <message>
        <source>The waveform editor provides control over the initial state or impulse that is used to start the string vibrating.  The buttons to the right of the graph will initialize the waveform to the selected type.  The &apos;?&apos; button will load a waveform from a file--only the first 128 samples will be loaded.

The waveform can also be drawn in the graph.

The &apos;S&apos; button will smooth the waveform.

The &apos;N&apos; button will normalize the waveform.</source>
        <translation>L&apos;editor della forma d&apos;onda fornisce il controllo sull&apos;impulso iniziale usato per far vibrare la corda. I pulsanti a destra del grafico predispongono una forma d&apos;onda del tipo selezionato. Il pulsante &apos;?&apos; carica la forma d&apos;onda da un file--verranno caricati solo i primi 128 campioni.

La forma d&apos;onda può anche essere disegnata direttamente sul grafico.

Il pulsante &apos;S&apos; ammorbisce la forma d&apos;onda.

Il pulsante &apos;N&apos; normalizza la forma d&apos;onda.</translation>
    </message>
    <message>
        <source>Vibed models up to nine independently vibrating strings.  The &apos;String&apos; selector allows you to choose which string is being edited.  The &apos;Imp&apos; selector chooses whether the graph represents an impulse or the initial state of the string.  The &apos;Octave&apos; selector chooses which harmonic the string should vibrate at.

The graph allows you to control the initial state or impulse used to set the string in motion.

The &apos;V&apos; knob controls the volume.  The &apos;S&apos; knob controls the string&apos;s stiffness.  The &apos;P&apos; knob controls the pick position.  The &apos;PU&apos; knob controls the pickup position.

&apos;Pan&apos; and &apos;Detune&apos; hopefully don&apos;t need explanation.  The &apos;Slap&apos; knob adds a bit of fuzz to the sound of the string.

The &apos;Length&apos; knob controls the length of the string.

The LED in the lower right corner of the waveform editor determines whether the string is active in the current instrument.</source>
        <translation>Vibed modella fino a nove corde che vibrano indipendentemente.  Il selettore &apos;Corda&apos; permettedi scegliere quale corda modificare.  Il selettore &apos;Imp&apos; sceglie se il grafico sarà l&apos;impulso dato o lo stato iniziale della corda.  Il selettore &apos;Ottava&apos; imposta l&apos;armonico a cui risuonerà la corda.

Il grafico permette di controllare l&apos;impulso (o lo stato iniziale) per far vibrare la corda.

La manopola &apos;V&apos; reogla il volume.  La manopola &apos;S&apos; regola la durezza della corda.  La manopola &apos;P&apos; regola la posiziona del plettro.  La manopola &apos;PU&apos; regola la posizione del pickup.

&apos;Pan&apos; e &apos;Detune&apos; non dovrebbero aver bisogno di spiegazioni.  La manopola &apos;Slap&apos; aggiunge un po&apos; di &quot;sporco&quot; al suono della corda.

La manopola &apos;Lunghezza&apos; regola la lunghezza della corda.

Il LED nell&apos;angolo in basso a destra sull&apos;editor della forma d&apos;onda determina se la corda è attiva nello strumento selezionato.</translation>
    </message>
    <message>
        <source>Enable waveform</source>
        <translation>Abilita forma d&apos;onda</translation>
    </message>
    <message>
        <source>Click here to enable/disable waveform.</source>
        <translation>Cliccando qui si abilita/disabilita la forma d&apos;onda.</translation>
    </message>
    <message>
        <source>String</source>
        <translation>Corda</translation>
    </message>
    <message>
        <source>The String selector is used to choose which string the controls are editing.  A Vibed instrument can contain up to nine independently vibrating strings.  The LED in the lower right corner of the waveform editor indicates whether the selected string is active.</source>
        <translation>Il selettore della Corda serve per scegliere su quale corda hanno effetto i controlli. Uno strumento Vibed può contenere fino a nove corde che indipendenti. Il LED nell&apos;angolo in basso a destra dell&apos;editor della forma d&apos;onda indica se la corda è attiva.</translation>
    </message>
    <message>
        <source>Sine wave</source>
        <translation>Onda sinusoidale</translation>
    </message>
    <message>
        <source>Triangle wave</source>
        <translation>Onda triangolare</translation>
    </message>
    <message>
        <source>Saw wave</source>
        <translation>Onda a dente di sega</translation>
    </message>
    <message>
        <source>Square wave</source>
        <translation>Onda quadra</translation>
    </message>
    <message>
        <source>White noise wave</source>
        <translation>Rumore bianco</translation>
    </message>
    <message>
        <source>User defined wave</source>
        <translation>Forma d&apos;onda personalizzata</translation>
    </message>
    <message>
        <source>Smooth</source>
        <translation>Ammorbidisci</translation>
    </message>
    <message>
        <source>Click here to smooth waveform.</source>
        <translation>Cliccando qui la forma d&apos;onda viene ammorbidita.</translation>
    </message>
    <message>
        <source>Normalize</source>
        <translation>Normalizza</translation>
    </message>
    <message>
        <source>Click here to normalize waveform.</source>
        <translation>Cliccando qui la forma d&apos;onda viene normalizzata.</translation>
    </message>
    <message>
        <source>&amp;Help</source>
        <translation>&amp;Aiuto</translation>
    </message>
    <message>
        <source>Use a sine-wave for current oscillator.</source>
        <translation>Utilizzare un&apos;onda sinusoidale per questo oscillatore.</translation>
    </message>
    <message>
        <source>Use a triangle-wave for current oscillator.</source>
        <translation>Utilizzare un&apos;onda triangolare per questo oscillatore.</translation>
    </message>
    <message>
        <source>Use a saw-wave for current oscillator.</source>
        <translation>Utilizzare un&apos;onda a dente di sega per questo oscillatore.</translation>
    </message>
    <message>
        <source>Use a square-wave for current oscillator.</source>
        <translation>Utilizzare un&apos;onda quadra per questo oscillatore.</translation>
    </message>
    <message>
        <source>Use white-noise for current oscillator.</source>
        <translation>Utilizzare rumore bianco per questo oscillatore.</translation>
    </message>
    <message>
        <source>Use a user-defined waveform for current oscillator.</source>
        <translation>Utilizzare un&apos;onda personalizzata per questo oscillatore.</translation>
    </message>
</context>
<context>
    <name>visualizationWidget</name>
    <message>
        <source>click to enable/disable visualization of master-output</source>
        <translation>cliccando si abilita/disabilita la visualizzazione dell&apos;uscita principale</translation>
    </message>
    <message>
        <source>Click to enable</source>
        <translation>Clicca per abilitare</translation>
    </message>
</context>
<context>
    <name>voiceObject</name>
    <message>
        <source>Voice %1 pulse width</source>
        <translation>Ampiezza pulse voce %1</translation>
    </message>
    <message>
        <source>Voice %1 attack</source>
        <translation>Attacco voce %1</translation>
    </message>
    <message>
        <source>Voice %1 decay</source>
        <translation>Decadimento voce %1</translation>
    </message>
    <message>
        <source>Voice %1 sustain</source>
        <translation>Sostegno voce %1</translation>
    </message>
    <message>
        <source>Voice %1 release</source>
        <translation>Rilascio voce %1</translation>
    </message>
    <message>
        <source>Voice %1 coarse detuning</source>
        <translation>Intonazione voce %1</translation>
    </message>
    <message>
        <source>Voice %1 wave shape</source>
        <translation>Forma d&apos;onda voce %1</translation>
    </message>
    <message>
        <source>Voice %1 sync</source>
        <translation>Sincronizzazione voce %1</translation>
    </message>
    <message>
        <source>Voice %1 ring modulate</source>
        <translation>Modulazione ring voce %1</translation>
    </message>
    <message>
        <source>Voice %1 filtered</source>
        <translation>Filtraggio voce %1</translation>
    </message>
    <message>
        <source>Voice %1 test</source>
        <translation>Test voce %1</translation>
    </message>
</context>
</TS>
