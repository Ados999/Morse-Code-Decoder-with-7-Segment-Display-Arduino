# Morse-Code-Decoder-with-7-Segment-Display-Arduino

🔠 Morse Code Decoder with 7-Segment Display (Arduino)

This Arduino project reads Morse code input from a button and displays the decoded letters on a 4-digit 7-segment display.
It measures how long the button is pressed to distinguish dots (.) and dashes (-), converts the sequence into a letter, and scrolls the result on the display.

⚙️ Main Features

Detects short (dot) and long (dash) button presses.

Automatically decodes Morse sequences after a short pause.

Displays decoded characters on a 4-digit 7-segment display.

Supports the full English alphabet (A–Z).

Optional DEBUG mode prints Morse input and decoded characters via Serial Monitor.

🧩 Components

Arduino board

FunShield (7-segment display + button)

One input button (for Morse tapping)

🧠 How It Works

Press the button shortly → .

Hold the button longer → -

After a pause (≈2 seconds), the program decodes the Morse sequence into a letter.

The letter appears on the 7-segment display, scrolling from right to left.
