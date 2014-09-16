MKNANDBOOT = ../mknandboot
SBU = ../DX_SBU.out
TWIST = ./twist.sh

TARGETS = bootrom.8.init.twisted bootrom.16.init.twisted

all: $(TARGETS)

bootrom.8.init: ../nand.image gen-vlsi-nandimage.sh
	./gen-vlsi-nandimage.sh 4096 224 $< $@

bootrom.8.init.twisted: bootrom.8.init twist.sh
	$(TWIST) $< > $@
	cat empty.8.init.twisted empty.8.init.twisted empty.8.init.twisted >> $@

bootrom.16.init: ../nand.image gen-vlsi-nandimage.sh
	./gen-vlsi-nandimage.sh 2048 64 $< $@

bootrom.16.init.twisted: bootrom.16.init twist.sh
	$(TWIST) $< > $@
	cat empty.16.init.twisted empty.16.init.twisted empty.16.init.twisted >> $@

#
# tooling
#

gen-vlsi-nandimage.sh: bin2pages BCH_Implementation vpjoin gen-otp-ecc

bin2pages: bin2pages.c
	gcc -Wall bin2pages.c -o bin2pages

BCH_Implementation: BCH_Implementation.cpp
	g++ -Wall BCH_Implementation.cpp -o BCH_Implementation

vpjoin: vpjoin.c
	gcc -Wall vpjoin.c -o vpjoin

gen-otp-ecc: gen-otp-ecc.c
	gcc -Wall gen-otp-ecc.c -o gen-otp-ecc

clean:
	rm -f *.o *.bin *.image *.map genbroken hello hello2 prenand *.txt bootrom.{8,16}.*
