#!/usr/bin/python

import random, sys
from optparse import OptionParser

class comm:
	def __init__(self, options, files, parser):
		self.files = files
		self.options = options
                self.parser = parser
	def commFiles(self):
                f1 = self.files[0]
                if f1 == "-":
                        lines1 = list(sys.stdin.readlines())
                else:
                        f = open(f1, 'r')
                        l1 = f.readlines()
                        f.close()
                        lines1 = list(l1)
                f2 = self.files[1]
                if f2 == "-":
                        lines2 = list(sys.stdin.readlines())
                else:
                        f = open(f2, 'r')
                        l2 = f.readlines()
                        f.close()
                        lines2 = list(l2)
                        
                if not self.options.unsorted:
                        if (lines1 != sorted(lines1)) or (lines2 != sorted(lines2)):
                                self.parser.error("Files need to be sorted or "\
                                                  "-u option should be used")
                
                if self.options.unsorted:
                        for line1 in lines1:
                                for line2 in lines2:
                                        if line1 == line2:
                                                lines2.remove(line2)
                                                if not self.options.no3:
                                                        if not self.options.no1:
                                                                if not self.options.no2:
                                                                        sys.stdout.write("\t\t" + line1)
                                                                else:
                                                                        sys.stdout.write("\t" + line1)
                                                        else:
                                                                if not self.options.no2:
                                                                        sys.stdout.write("\t" + line1)
                                                                else:
                                                                        sys.stdout.write(line1)
                                                break
                                        else:
                                                if not self.options.no1:
                                                        sys.stdout.write(line1)
                                                break;


                        for line2 in lines2:
                                if not self.options.no2:
                                        if not self.options.no1:
                                                sys.stdout.write("\t" + line2)
                                        else:
                                                sys.stdout.write(line2)
                else:
                        c1 = 0
                        c2 = 0
                        while c1 != len(lines1) and c2 != len(lines2):
                                if lines1[c1] < lines2[c2]:
                                        if not self.options.no1:
                                                sys.stdout.write(lines1[c1])
                                        c1 += 1
                                elif lines1[c1] > lines2[c2]:
                                        if not self.options.no2:
                                                if not self.options.no1:
                                                        sys.stdout.write("\t" + lines2[c2])
                                                else:
                                                        sys.stdout.write(lines2[c2])
                                                c2+=1
                                elif lines1[c1] == lines2[c2]:
                                        if not self.options.no3:
                                                if not self.options.no1:
                                                        if not self.options.no2:
                                                                sys.stdout.write("\t\t" + lines1[c1])
                                                        else:
                                                                sys.stdout.write("\t" + lines1[c1])
                                                        
                                        
                                                else:
                                                        if not self.options.no2:
                                                                sys.stdout.write("\t" + lines1[c1])
                                                        else:
                                                                sys.stdout.write(lines1[c1])
                                        c1 += 1
                                        c2 += 1
                        
                        if c1 == len(lines1):
                                while c2 != len(lines2):
                                        if not self.options.no2:
                                                if not self.options.no1:
                                                        sys.stdout.write("\t" + lines2[c2])
                                                else:
                                                        sts.stdout.write(lines2[c2])
                                        c2 += 1
                        else:
                                while c1 != len(lines1):
                                        if not self.options.no1:
                                                sys.stdout.write(lines1[c1])
                                        c1 += 1
                                                           
                                                
                                                


def main():
	usage_msg = """%prog [-123] [-u] FILE1 FILE2
	Select or reject lines common to two files
	Options:
	-1 Suppress the output column of lines unique to FILE1
	-2 Suppress the output column of lines unqiue to FILE2
	-3 Suppress the output column of lines duplicated in FILE1 and FILE2
	-u Allows FILE1 and FILE2 to be unsorted
	"""
	
	parser = OptionParser(usage=usage_msg)
	
	parser.add_option(
		"-1", action="store_true", default=False,
		dest="no1",
		help="Suppress the output column of lines unique to FILE1")
	parser.add_option(
		"-2", action="store_true", default=False,
		dest="no2",
		help="Suppress the output column of lines unique to FILE2")
	parser.add_option(
		"-3", action="store_true", default=False,
		dest="no3",
		help="Suppress output of lines duplicated in FILE1 and FILE2")
	parser.add_option(
		"-u", action="store_true", default=False, 
		dest="unsorted", 
		help="Allows FILE1 and FILE2 to be unsorted")

	options, args = parser.parse_args(sys.argv[1:])
	
        if len(args) != 2:
                parser.error("There must be two operands")
        

        try:
                com = comm(options, args, parser)
                com.commFiles()
        except IOError as err:
                parser.error("I/O error({0}):{1}".format(errno, strerror))

if __name__ == "__main__":
	main()

