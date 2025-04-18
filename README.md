# Platformio

    # Install ch32v platform
    pio pkg install -g -p https://github.com/Community-PIO-CH32V/platform-ch32v.git

    # Add udev rules
    sudo tee -a /etc/udev/rules.d/30-mcu.rules > /dev/null << 'EOF'
    SUBSYSTEM=="usb", ATTR{idVendor}="1a86", ATTR{idProduct}=="8010", GROUP="uucp", SYMLINK+="wchlink"
    SUBSYSTEM=="usb", ATTR{idVendor}="4348", ATTR{idProduct}=="55e0", GROUP="uucp", SYMLINK+="ch32v203"
    SUBSYSTEM=="usb", ATTR{idVendor}="1a86", ATTR{idProduct}=="8012", GROUP="uucp"
    EOF

# Documentation

Example projects: https://github.com/openwch/ch32_training_documentation/tree/main  
OpenWCH HAL: https://github.com/openwch/ch32v20x  
OpenWCH examples: https://github.com/openwch/ch32v20x/tree/main/EVT/EXAM  
QuingkeV4 processor manual: https://www.wch-ic.com/downloads/file/367.html?time=2025-04-15%2015:15:33&code=DneYBmkOQNWdSke5iuXjQd7zYjmJXwbvy2PH88og  
ch32v203 manual: https://www.wch-ic.com/downloads/file/354.html?time=2025-04-14%2020:27:44&code=My60CX1TGv95rvuDKBOolMfk4qKYA0tfYlKqRevT  
