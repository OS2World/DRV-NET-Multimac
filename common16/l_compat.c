#include "Dev16lib.h"
#include "base.h"
#include "pci_regs.h"
#include "ndis.h"
#include "mii.h"
#include "l_compat.h"
#include "driver.h"
#include "sas.h"

u32 jiffies;

int pci_read_config_word(struct pci_dev *pDev, u16 reg, u16 far *pval)
{
    return PciReadConfig(pDev->BusDevFunc, reg, 2, pval);
}

void pci_read_config_byte(struct pci_dev *pDev, u16 reg, u8 far *pval)
{
    PciReadConfig(pDev->BusDevFunc, reg, 1, pval);
}


int pci_write_config_word(struct pci_dev *pDev, u16 reg, u16 val)
{
    return PciWriteConfig(pDev->BusDevFunc, reg, 2, val);
}

void pci_write_config_byte(struct pci_dev *pDev, u16 reg, u8 val)
{
    PciWriteConfig(pDev->BusDevFunc, reg, 1, val);
}

static bool pcie_capability_reg_implemented(struct pci_dev *dev, int pos)
{
    if (!pci_is_pcie(dev))
        return false;

    switch (pos) {
    case PCI_EXP_FLAGS:
        return true;
    case PCI_EXP_DEVCAP:
    case PCI_EXP_DEVCTL:
    case PCI_EXP_DEVSTA:
        return pcie_cap_has_devctl(dev);
    case PCI_EXP_LNKCAP:
    case PCI_EXP_LNKCTL:
    case PCI_EXP_LNKSTA:
        return pcie_cap_has_lnkctl(dev);
    case PCI_EXP_SLTCAP:
    case PCI_EXP_SLTCTL:
    case PCI_EXP_SLTSTA:
        return pcie_cap_has_sltctl(dev);
    case PCI_EXP_RTCTL:
    case PCI_EXP_RTCAP:
    case PCI_EXP_RTSTA:
        return pcie_cap_has_rtctl(dev);
    case PCI_EXP_DEVCAP2:
    case PCI_EXP_DEVCTL2:
    case PCI_EXP_LNKCAP2:
    case PCI_EXP_LNKCTL2:
    case PCI_EXP_LNKSTA2:
        return pcie_cap_version(dev) > 1;
    default:
        return false;
    }
}

/*
 * Note that these accessor functions are only for the "PCI Express
 * Capability" (see PCIe spec r3.0, sec 7.8).  They do not apply to the
 * other "PCI Express Extended Capabilities" (AER, VC, ACS, MFVC, etc.)
 */
int pcie_capability_read_word(struct pci_dev *dev, int pos, u16 far *val)
{
    int ret;

    *val = 0;
    if (pos & 1)
        return -EINVAL;

    if (pcie_capability_reg_implemented(dev, pos)) {
        ret = pci_read_config_word(dev, pci_pcie_cap(dev) + pos, val);
        /*
         * Reset *val to 0 if pci_read_config_word() fails, it may
         * have been written as 0xFFFF if hardware error happens
         * during pci_read_config_word().
         */
        if (ret)
            *val = 0;
        return ret;
    }

    /*
     * For Functions that do not implement the Slot Capabilities,
     * Slot Status, and Slot Control registers, these spaces must
     * be hardwired to 0b, with the exception of the Presence Detect
     * State bit in the Slot Status register of Downstream Ports,
     * which must be hardwired to 1b.  (PCIe Base Spec 3.0, sec 7.8)
     */
    if (pci_is_pcie(dev) && pos == PCI_EXP_SLTSTA &&
         pci_pcie_type(dev) == PCI_EXP_TYPE_DOWNSTREAM) {
        *val = PCI_EXP_SLTSTA_PDS;
    }

    return 0;
}

int pcie_capability_write_word(struct pci_dev *dev, int pos, u16 val)
{
    if (pos & 1)
        return -EINVAL;

    if (!pcie_capability_reg_implemented(dev, pos))
        return 0;

    return pci_write_config_word(dev, pci_pcie_cap(dev) + pos, val);
}

int pcie_capability_clear_and_set_word(struct pci_dev *dev, int pos, u16 clear, u16 set)
{
    int ret;
    u16 val;

    ret = pcie_capability_read_word(dev, pos, &val);
    if (!ret) {
        val &= ~clear;
        val |= set;
        ret = pcie_capability_write_word(dev, pos, val);
    }

    return ret;
}

fptr kmalloc(u32 AllocSize, u16 Unused)
{
    u32 PhysAdr;
    fptr Vadr;

    if (AllocSize > (0x10000L-4)) return 0;

    AllocSize += 4; /* allow room for physical address */

    PhysAdr = AllocPhysMemory(AllocSize);
    if (!PhysAdr) return 0;

    Vadr = MapPhysToVirt(PhysAdr, AllocSize);

    *(fpu32)Vadr = PhysAdr;

    return (fptr)((fpu8)Vadr + 4);
}

void kfree(fptr Vadr)
{
    u32 PhysAdr;

    PhysAdr = *(((fpu32)Vadr)-1);
    DevHelp_FreeGDTSelector((u32)Vadr >> 16);
    DevHelp_FreePhys(PhysAdr);
}

fptr kzalloc(u32 AllocSize, u16 Unused)
{
    fpchar Vadr;
    u16 i;

    if (AllocSize >= 0x10000L) return 0;

    Vadr = (fpchar)kmalloc(AllocSize, Unused);
    if (!Vadr) return 0;

    for (i=0; i<AllocSize; i++) Vadr[i] = 0;

    return Vadr;
}

typedef struct _FILESTATUS3 {
  u16     fdateCreation;    /*  Date of file creation. */
  u16     ftimeCreation;    /*  Time of file creation. */
  u16     fdateLastAccess;  /*  Date of last access. */
  u16     ftimeLastAccess;  /*  Time of last access. */
  u16     fdateLastWrite;   /*  Date of last write. */
  u16     ftimeLastWrite;   /*  Time of last write. */
  u32     cbFile;           /*  File size (end of data). */
  u32     cbFileAlloc;      /*  File allocated size. */
  u32     attrFile;         /*  Attributes of the file. */
} FILESTATUS3;

static u8 FirmwareBuffer[6000];

/**
 * Returns <0 on failure.
 */
short request_firmware(const struct firmware **fw, const char *Name, struct net_device *dev)
{
    u16 rc;
    u16 hFile;
    u16 Action;
    u16 usLen;
    u16 usActual;
    struct firmware *pFw;
    FILESTATUS3 StatBuf;
    char FileName[64];

    /* Open the firmware file */
    strcpy(FileName, mg_FwDir);
    strcat(FileName, "/");
    strcat(FileName, Name);
    TraceBuf(0x0006, strlen(FileName)+1, (fptr)FileName);
    rc = DosOpen(FileName, &hFile, &Action, 0L, 0, 1, 0x2020, 0L);
    if (rc) return -1;

    rc = DosQFileInfo(hFile, 1, &StatBuf, sizeof(StatBuf));
    if (rc)
    {
        DosClose(hFile);
        return -1;
    }

    usLen = StatBuf.cbFile;
    if ((usLen + sizeof(struct firmware)) > sizeof(FirmwareBuffer))
    {
        DosClose(hFile);
        return -1;
    }

    pFw = (struct firmware *)&FirmwareBuffer;
    pFw->size = usLen;
    pFw->data = ((const u8 *)(pFw)) + sizeof(struct firmware);

    rc = DosRead(hFile, (fptr)pFw->data, usLen, &usActual);
    DosClose(hFile);
    if (rc)
    {
        return -1;
    }

    *fw = pFw;
    return 0;
}

void release_firmware(const struct firmware *fw)
{
}


