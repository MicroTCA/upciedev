# upciedev
The basic functionality of the PCI Express device  does not depend on device type and could be common for all drivers. The Linux Device Driver Model allows modules stacking, that basically means one module can use the symbols defined in other modules. Using the stacking and independence of  the basic functionality of the PCI Express device allows us to split PCI Express device driver into multiple parts. The driver for current device will use PCI Express driver common part provided by the low level driver. The low level module provides all common structures and functions for PCI Express communication.
Such flexibility will facilitate the tasks of creation and supporting of the device drivers, on the other hand it will lead to the principle ”write for one use for all”  at the level of user programming.

universal PCIe driver, provides the base PCIe functionality to be used by the top level drivers.
It is kept adapted for PCI Express HotPlug and Standard Hardware API guidelines of PICMIG.

