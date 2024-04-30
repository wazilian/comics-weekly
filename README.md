
## comics-weekly

#### EXE Parameters:
```
comics-weekly <Metron username> <Metron password> <text files directory> <list of comic publishers>
```
#### EXE Example:
```
comics-weekly username password /var/www/html/txt DC Marvel Skybound
```
### INSTALL
* Clone a copy of the Mbed TLS project (https://github.com/Mbed-TLS/mbedtls) and edit the comics-weekly Makefile variable TLS_TREE to point to the Mbed TLS directory
* Run make
* Copy the comics-weekly executable to a directory of choice
* Configre comics-weekly to run at midnight on Sunday
```
0 0 * * 0 root /usr/bin/comics-weekly API_username API_password /var/www/html/txt DC Markvel Skybound
```
* Copy the index.php and style.css files to a root or subdirectory of your webserver
* Edit the $output_dir variable in index.php that points to the output text file location (I'll lump this into a 'make install' in a future revision)
### ABOUT
There are plenty of apps and websites that can show you the current and upcoming comic books that will be on shelves in comic shops. I just don't like them. So, I created this project in a format that I enjoy and I get to use my software skills to accomplish this goal.

New Comic Book Day is Wednesdays. Don't get me started on DC Comics and their decision in 2020 to move their new book day to Tuesdays (Grr). DC is moving back to Wednesdays this summer (July 2024).

To obtain the list of comics for the current week, previous week, and the comics coming the following week, I chose to write a C program. I chose C for a few reasons. Its my favorite language to code in. Plus, I wanted to use the TLS library, Mbed TLS (https://github.com/Mbed-TLS/mbedtls), to make an HTTP request to a comic book API. I've used this library in the past for a work project and really enjoyed using it, especially for an embedded project. They had made some changes to the library since I last used it, so I wanted to take the opportunity to become familiar with the library as it is now.

I chose the API over at Metron (https://metron.cloud/). To use this API, you will need an account, but its free. There are other comic book APIs out there, but this is the one I've selected. The Metron API has a lot of options and fields. Not all of which I'll utilize, certainly at the start. Plus, the creator, Brian Pepple, was a big help to me as I was working through some tasks utilizing the API. Shout out to Brian!

There are plenty of projects out there, but to parse the JSON file in C that is received from the Metron API, I'm using the project, cJSON (https://github.com/DaveGamble/cJSON).

Once I have the list in the C program, I'll write the list to 3 standard ASCII text files based on the previous, current, and next week structure. For now, I'm not separating the lists per publisher.

I want to display the information in the ASCII text files graphically in a format I find sufficient. I'm not sure yet if I'll take that to an app in the future, but a webpage that is local to my home network will certainly work for now. Of course, that won't help if I want the information remotely, unless I do some port forwarding on my home router. Not interested in doing that at this time. Again, this is mostly just for me. Its a simple webpage using PHP to read the text files and display that information in a tabular form. Simple and to the point for my needs. Each issue is a HTML link back to the Metron site for the given issue which contains even more information about the given issue.

At this time, I do not want to open this project for collaboration. If anyone wishes to take it and modify it to their needs, please, by all means. I can see so many directions and possible changes other developers could change to suit them. For example, the JSON file from the API could be written, instead of parsed, to file and allow an app or webpage to decode the JSON file and display the data in a format that works for them. Another idea is to supply a pull list to the C application and use that list to only show those comic titles for the previous, current, and next week lists.

### NOTES
Couple of notes about the external libraries used in the project:

* cJSON: I only copied the cJSON.c and cJSON.h files from that project and included them in the src directory of this project
* Mbed TLS: Since this library is bigger, subject to change with frequency, and I've used it on other projects on my systems, I have a Makefile variable that should be edited to the location of the Mbed TLS directory

<br/>
wazilian
