To check your storage or disk space usage (ROM/HDD/SSD) as a regular user in Linux, you can use built-in terminal tools or a graphical interface.
The fastest, most reliable way to check your disk space from the terminal is by using the df and du commands.
## 1. View Entire Disk and Partition Status (df)
To see how full your storage drives and partitions are, use the Disk Free (df) command. Adding the -h flag makes it human-readable (showing sizes in GB and MB instead of blocks):

df -h


* What to look for: Look for the row that has / (the root directory) or /home/yourusername in the Mounted on column. The Use% column shows exactly how full that drive is.
* Filter to only see real physical drives: Run df -h | grep -v tmpfs to hide temporary system files and focus only on your actual HDD/SSD partitions.

## 2. View Space Used by Your Personal Files (du)
If you want to know how much disk space your specific user folders (like Documents, Downloads, or your entire Home folder) are occupying, use the Disk Usage (du) command:

* Check the grand total of your Home directory:

du -sh ~

(The ~ symbol automatically points to your current user's home folder).
* See a list of your top-level folders and their sizes:

du -h --max-depth=1 ~ | sort -h

This will list all folders inside your home directory and sort them from smallest to largest, making it incredibly easy to spot what is hogging your space.

## 3. The Interactive Terminal Way (ncdu)
If you want an easy, interactive way to browse your hard drive and delete large files straight from the terminal, ncdu (NCurses Disk Usage) is the best tool available.

   1. Install it (usually requires root once, or ask an admin):
   * Ubuntu/Debian: sudo apt install ncdu
      * Fedora/RHEL: sudo dnf install ncdu
   2. Scan your home folder:
   
   ncdu ~
   
   You can use the arrow keys to navigate through folders and press d to delete files you no longer need.

## 4. The Graphical Way (GUI)
If you are running a Linux desktop environment (like GNOME or KDE), you can open these pre-installed apps:

* Disks / GParted: Shows partitions, health, and total drive size.
* Disk Usage Analyzer (also known as Baobab): Provides a visual pie chart or ring chart breaking down exactly what folders are taking up room on your HDD.

Are you running low on space and need help finding and safely deleting large hidden cache files (like browser or package manager caches), or are you trying to check the health status of a specific external drive?

