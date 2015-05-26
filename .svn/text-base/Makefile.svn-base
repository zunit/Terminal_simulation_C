all: ext2_ls ext2_rm  ext2_cp ext2_ln  ext2_mkdir

ext2_ls :  ext2_ls.o helper.o
	gcc -Wall -g -o ext2_ls $^

ext2_rm : ext2_rm.o helper.o
	gcc -Wall -g -o ext2_rm $^

ext2_ln : ext2_ln.o helper.o
	gcc -Wall -g -o ext2_ln $^

ext2_cp : ext2_cp.o helper.o
	gcc -Wall -g -o ext2_cp $^

ext2_mkdir : ext2_mkdir.o helper.o
	gcc -Wall -g -o ext2_mkdir $^

%.o : %.c ext2.h helper.h disk.h
	gcc -Wall -g -c $<

clean : 
	rm -f *.o ext2_ls *~
	rm -f *.o ext2_cp *~
	rm -f *.o ext2_ln *~
	rm -f *.o ext2_rm *~
	rm -f *.o ext2_mkdir *~
