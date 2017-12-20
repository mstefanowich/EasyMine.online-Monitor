# **EasyMine.online Monitor**

I got tired of checking vertcoin.easymine.online so much to check the progress of my miner(s). So, I spoke with the owner of easymine and decided to make my own little tool to monitor my progress. I also decided to publish it online so others can use it :)

### **NOTE**
* Build Version 0.0 is sloppily written and MAY have bugs. If you encounter any bugs please let me know. I will fix them ASAP. This has only been tested on Windows 10.

### **TO-DO**

12/20/17
* Fix sloppy JSON parsing
* Improve overall aesthetics
* Convert H to MH or GH
* Make sure runs on all versions of Windows
* Include segment to list blocks currently being mined

## Getting Started

Download/Clone/??? the main.c file, start it up in VS as a blank C++ project and include wininet.lib under linked options. Which ever OR download the .exe file listed here.

#### **Prerequisites**

Works on Windows ONLY.

### **How-To-Use**

    * Start in cmd as C:\Path\To\File\easymineagent.exe VtP1ea9gWvjGzzA8x4uJD68vNZzbjDGGuq VnjFZ64GFGRejFrizkd27JASdst8BYxWoE
    * Parameters are to be passed as seen above, no commas, slashes, hyphens, etc. Just pass the wallet address.

## Built With

* [Visual Studio 2017](https://www.visualstudio.com/vs/whatsnew/)
* [Microsoft Windows API](https://msdn.microsoft.com/en-us/library/aa383723(VS.85).aspx)

## Authors

* **Mathew A. Stefanowich** - *Initial work*

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

* nzsquirrel for the API tips
* StackOverflow for the JSON parsing tips and tricks
* VTC for that sweet, sweet $$$
