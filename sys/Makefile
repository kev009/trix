SOURCE = 	h/*.h \
		low/*.s \
		kernel/*.s kernel/*.[cd] \
		filesys/*.h filesys/*.c \
		system/*.c \
		dev/*.s dev/*.c \
		kdev/*.c \
		sdu/*.c \
		lib/*.s lib/*.c

error:
		@echo	"make what?"

update:
		cd low ; make update
		cd dev ; make update
		cd kdev ; make update
		cd filesys ; make update
		cd kernel ; make update
		cd lib ; make update
		cd sdu ; make update
		cd system ; make update

list:
		prd ${SOURCE} | dover

save:
		-mv trix.a trix.A
		ar cr trix.a ${SOURCE}
