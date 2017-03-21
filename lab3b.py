class Inode:
	def __init__(self, inode_num,  number_of_links):
			self.inode_num = inode_num
			self.ref_list = []
			self.number_of_links = number_of_links
			self.pointers = []
				



class Block:
	def __init__(self, block_num):
		self.block_num = block_num
		self.ref_list = []




check = open('lab3b_check.txt','w')
superblock =open('super.csv','r')
group_descriptor = open('group.csv','r')
bitmap = open('bitmap.csv','r')
inode = open('inode.csv','r')
directory = open('directory.csv','r')
indirect = open('indirect.csv','r')



total_inodes = 0
total_blocks = 0
block_size = 0
blocks_per_group = 0
inodes_per_group = 0

free_block = []
free_inode = []
allocated_block = {}
allocated_inode = {}


def register_block(block_num, inode_num, i_block_num, entry_num):
	ref_list = []
	if (block_num != 0):
		if(block_num in allocated_block):
			allocated_block[block_num].ref_list.append([inode_num, i_block_num, entry_num])
		else:
			allocated_block[block_num] = Block(block_num)
			allocated_block[block_num].ref_list.append([inode_num, i_block_num, entry_num])


for entry in superblock:
	line = entry.rstrip('\n').split(',')
	total_inodes = int(line[1])
	total_blocks = int(line[2])
	block_size = int(line[3])
	blocks_per_group = int(line[5])
	inodes_per_group = int(line[6])

block_bitmap = list()
inode_bitmap = list()

for entry in group_descriptor:
	line = entry.rstrip('\n').split(',')
	block_bitmap.append(int(line[5],16))
	inode_bitmap.append(int(line[4],16))



for entry in bitmap:
	line = entry.rstrip('\n').split(',')
	if(int(line[0],16) in block_bitmap):
		free_block.append(int(line[1]))
	if(int(line[0],16) in inode_bitmap):
		free_inode.append(int(line[1]))	


for entry in inode:
	line = entry.rstrip('\n').split(',')
	direct_block_pointers = line[11:22]
	allocated_inode[int(line[0])] = Inode(int(line[0]),int(line[5]))
	num_blocks = int(line[10])
	cur_block = 0;
#REGULAR BLOCKS
	iteration = 0
	for block_entry in direct_block_pointers:
		if (cur_block >= num_blocks):
			break
		n_block = int(block_entry,16)
		register_block(n_block, int(line[0]),0,iteration)
		allocated_inode[int(line[0])].pointers.append(n_block)
		if(n_block > total_blocks or n_block == 0):
			check.write("INVALID BLOCK < " + str(n_block) + " > IN INODE < " + str(line[0]) + " > ENTRY < " + str(iteration) + " >\n")
		cur_block+=1
		iteration+=1


#SINGLE INDIRECT BLOCKS
	if (num_blocks>12 and cur_block < num_blocks):
		single_indirect = int(line[23],16)
		if(single_indirect > total_blocks or single_indirect == 0):
				check.write("INVALID BLOCK < " + str(single_indirect) + " > IN INODE < " + str(line[0]) + " > ENTRY < 12 >\n")
		if(single_indirect != 0):
			allocated_inode[int(line[0])].pointers.append(single_indirect)
			cur_block+=1
			for i_entry in indirect:
				i_line = i_entry.rstrip('\n').split(',')
				i_ptr = int(i_line[2],16)
				if i_ptr in allocated_inode:
					register_block(i_ptr, allocated_inode[i_ptr], int(i_line[0],16),int(i_line[1]))
				cur_block+=1

				

#DOUBLY INDIRECT BLOCKS	
	if (num_blocks>268):
		double_indirect = int(line[24],16)
		if(double_indirect > total_blocks):
				check.write("INVALID BLOCK < " + str(double_indirect) + " > IN INODE < " + str(line[0]) + " > ENTRY < 13 >\n")
		if(double_indirect != 0):
			allocated_inode[int(line[0])].pointers.append(double_indirect)


#TRIPLY INDIRECT BLOCKS
	if (num_blocks>65536):
		triple_indirect = int(line[25],16)
		if(triple_indirect > total_blocks):
				check.write("INVALID BLOCK < " + str(triple_indirect) + " > IN INODE < " + str(line[0]) + " > ENTRY < 13 >\n")
		if(triple_indirect != 0):
			allocated_inode[int(line[0])].pointers.append(triple_indirect)

directory_dict = {}

for entry in directory:
		line = entry.rstrip('\n').split(',')
		entry_name = line[5]
		if (int(line[0]) != int(line[4]) or int(line[0]) == 2):
			directory_dict[int(line[4])] = int(line[0])

		if(int(line[4])in allocated_inode):
			allocated_inode[int(line[4])].ref_list.append([int(line[0]),int(line[1])])
		else:
			check.write("UNALLOCATED INODE < " + line[0] + " > REFERENCED BY DICTIONARY < " + line[0] + " > ENTRY < " + line[1] + " >\n")

		if (entry_name == '"."'):
			if(int(line[0]) != int(line[4])):
				check.write("INCORRECT ENTRY IN < " + line[0] + " > NAME < . > LINK TO < " + line[4] + " > SHOULD BE < " + line[0] + " >\n")
		if(entry_name == '".."'):
			if (directory_dict[int(line[0])] != int(line[4])):
				check.write("INCORRECT ENTRY IN < " + line[0] + " > NAME < .. > LINK TO < " + line[4] + " > SHOULD BE < " + str(directory_dict[int(line[0])]) + " >\n")

for i,entry in allocated_inode.items():
	ref_temp = entry.ref_list
	if(len(ref_temp) == 0 and entry.inode_num > 10):
		check.write("MISSING INODE < " + str(entry.inode_num) + " > SHOULD BE IN FREE LIST < " + str(inode_bitmap[entry.inode_num/inodes_per_group]) + " >\n")

	elif(len(ref_temp)!= entry.number_of_links):
		check.write("LINKCOUNT < " + str(entry.inode_num) + " > IS < " + str(entry.number_of_links) + " > SHOULD BE < " + str(len(ref_temp)) + " >\n")


for i,entry in allocated_block.items():       
  ref_temp = len(entry.ref_list)
  if(ref_temp>1):
	check.write("MULTIPLY REFERENCED BLOCK < " + str(entry.block_num) + " > BY")
	for refs in entry.ref_list:
		if(refs[1] == 0):
			check.write(" INODE < " + str(refs[0])+ " > ENTRY < " + str(refs[2]) + " >")
		else:
			check.write(" INODE < " + str(refs[0])+ " > INDIRECT BLOCK < " + str(refs[1]) + " > ENTRY < " + str(refs[2]) + " >")
	check.write("\n")

	
	
   
for free in free_block:
	if (free in allocated_block):
		check.write("UNALLOCATED BLOCK < " + str(allocated_block[free].block_num) + " > REFERENCED BY")
		for ref1 in allocated_block[free].ref_list:
			if(ref1[1] == 0):
				check.write(" INODE < " + str(ref1[0])+ " > ENTRY < " + str(ref1[2]) + " >")
			else:
				check.write(" INODE < " + str(ref1[0])+ " > INDIRECT BLOCK < " + str(ref1[1]) + " > ENTRY < " + str(ref1[2]) + " >")
		check.write("\n")
# for temp1 in inode:
# 		entry = temp1.rstrip('\n').split(',')
# 	for temp in inode:
# 		temp = entry.rstrip('\n').split(',')




superblock.close()
group_descriptor.close()
bitmap.close()
inode.close()
directory.close()
indirect.close()
check.close()

