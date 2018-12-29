#include <kern/e1000.h>
#include <inc/error.h>
// LAB 6: Your driver code here

//Global descriptor and packet arrays
#define debug 0
static tx_desc td[MAXTRANDESC];
static char tp[MAXPACKETLEN * MAXTRANDESC];//continuous memory


int attach_E1000(struct pci_func *func)
{
	pci_func_enable(func);
	//virtual mapping for E1000's BAR 0
	e1000addr = mmio_map_region(func->reg_base[0], func->reg_size[0]);
	cprintf("STATUS: 0x%x\n", e1000addr[E1000_STATUS]);
	//alignment check
	if(sizeof(td) % 128 != 0)
		panic("Alignment check for transmit descriptor failed");
	init_transmit(); //initialize transmit registers


	return 0;
}
void init_transmit()
{
/*	int i;
        //register intialiazation according to section 14.5
        e1000addr[E1000_TDBAL] = PADDR(td);
        //e1000addr[E1000_TDBAH] =      
        e1000addr[E1000_TDLEN] = sizeof(td);
        e1000addr[E1000_TDH] = 0;
        e1000addr[E1000_TDT] = 0;
        e1000addr[E1000_TCTL] |= E1000_TCTL_EN;
        e1000addr[E1000_TCTL] |= E1000_TCTL_PSP;
        e1000addr[E1000_TCTL] |= 0x00000010;//CT
        e1000addr[E1000_TCTL] |= 0x00040000;//COLD 40h << 12
        e1000addr[E1000_TIPG] = 0x00000010;

	//point each desc to base addr of corresponding pkt
	for(i = 0; i < MAXTRANDESC; i++) {
		td[i].addr = PADDR(&tp[i * MAXPACKETLEN]);
		//td[i].status = 0;// TBD
		//td[i].cmd = 0;
		//transmit function won't work without setting the
		//DD bit, and for that RS bit should be se already
		td[i].status |= E1000_TXD_CMD_RS;
		td[i].cmd |=  E1000_TXD_STAT_DD;
	}
*/
  int i;
    // perform transmit initialization
    e1000addr[E1000_TDBAL] = PADDR(td);
    e1000addr[E1000_TDLEN] = sizeof(td);
    e1000addr[E1000_TDH] = 0;
    e1000addr[E1000_TDT] = 0;
    e1000addr[E1000_TCTL] |= E1000_TCTL_EN;
    e1000addr[E1000_TCTL] |= E1000_TCTL_PSP;
    e1000addr[E1000_TCTL] |= E1000_TCTL_CT_INIT;
    e1000addr[E1000_TCTL] |= E1000_TCTL_COLD_INIT;
    e1000addr[E1000_TIPG] |= E1000_TIPG_INIT;
    //e1000addr[E1000_TIPG] = 0x00000010;
    // init transmit descriptors
    for (i = 0; i < MAXTRANDESC; i++) {
        td[i].addr = PADDR(&tp[i * MAXPACKETLEN]);
        td[i].cmd |= E1000_TXD_CMD_RS;
        td[i].status |= E1000_TXD_STAT_DD;
    }

}

int transmit(void *addr, uint16_t length)
{
/*	int temp_tail = e1000addr[E1000_TDT];
	
	//check if next descriptor is free
	if(td[temp_tail].status & E1000_TXD_STAT_DD) //can be used
	{
		memcpy(KADDR(td[temp_tail].addr), va, len);
		td[temp_tail].status &= ~E1000_TXD_STAT_DD;
		td[temp_tail].length = len;
		temp_tail = (temp_tail + 1) / MAXTRANDESC; //loop back if end of queue
		e1000addr[E1000_TDT] = temp_tail; //update TDT	
		return 0; //successful
	}
	
	return -1; //drop packet
*/
    int tail = e1000addr[E1000_TDT];

    if (length > MAXPACKETLEN)
        return -E_INVAL;

    
        cprintf("transmit tail: %0x\n", tail);
        cprintf("transmit tail status %0x\n", td[tail].status);
   

    if (!(td[tail].status & E1000_TXD_STAT_DD)) {
        // if the dd field is not set, the desc is not free
        // we simply drop the packet in this case
        cprintf("Transmit descriptors array is full\n");
        return -1;
    }

    memmove(KADDR(td[tail].addr), addr, length);
    td[tail].length = length;
    td[tail].status &= ~E1000_TXD_STAT_DD;
    td[tail].cmd |= E1000_TXD_CMD_EOP;
    e1000addr[E1000_TDT] = (tail + 1) % MAXTRANDESC;
    return 0;


}
