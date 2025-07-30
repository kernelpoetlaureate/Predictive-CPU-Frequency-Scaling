obj-m += predictive-cpufreq.o

predictive-cpufreq-objs := predictive_governor.o predictive_model.o \
                          metric_collector.o pattern_recognizer.o

KDIR := /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean

install:
	make -C $(KDIR) M=$(PWD) modules_install
	depmod -a
