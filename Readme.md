# LizardLines

LizardLines is an small utility which allows you to see the change in line count of a project over time. It will automatically count the number of lines in a project and save the count. Users can then export the count and view the change over time.

# Features

  - Saves the line count of a project on the first compile of each day.
  - Easily integrates into any build process.
  - Can specify what types of file to count.
  - Export the data as a .csv

# Usage
To use simply integrate into your build process. Run the program on build.
```
./LizardLines.exe PathToProjectFolder OutputFile.ll 
```
You have the option to specify specific file types. If no specific file types are given, then java, cpp, h, py files will be counted.
```
./LizardLines.exe PathToProjectFolder OutputFile.ll java cpp
```
The total number of lines in the project will be counted and saved to the file. This will only happen on the first compile of each day.
To output the data simply run.
```
./LizardLines.exe -csv FileName.csv
```