# Parse recorded data in .raw files

    for f in *.raw; do echo "$f"; ./parse.py $f; echo ""; done

# Read data from /dev/ttyACM1 and parse it

    stty -F /dev/ttyACM1 cs8 9600 ignbrk -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts
    tail -f /dev/ttyACM1 | ./parse.py -
