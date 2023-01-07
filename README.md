# The ODD language
## Usage
The compiler bash alias is 'codd' to use the compiler run codd < input .odd file > < your output .ll file> if you don't want to go through the hassle of copiling yourself you can put your code in the test.odd file and run make calr in the command line

#### Example of compillation<br>
`codd example.odd example.ll`<br>
`llc -filetype=obj example.ll -o example.o`<br>
Clang is used as the linker ld will also suffice<br>
`clang -o example example.o libodd/object_files/oddio.o -lm`<br>
`./example`

##### Note: It's a work in progress