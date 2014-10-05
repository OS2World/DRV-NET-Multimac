 
#include "hw_common.h"

int NIC_DEVICE::Suspend()
{
	/*
	 * This function is called when system goes into suspend state
	 * mac80211 will call iwl_mac_stop() from the mac80211 suspend function
	 * first but since iwl_mac_stop() has no knowledge of who the caller is,
	 * it will not call apm_ops.stop() to stop the DMA operation.
	 * Calling apm_ops.stop here to make sure we stop the DMA.
	 */
///PM later, after debugging///	iwl_apm_stop(priv);

	return 0;
}

int NIC_DEVICE::Resume()
{
	bool hw_rfkill = false;

	/*
	 * We disable the RETRY_TIMEOUT register (0x41) to keep
	 * PCI Tx retries from interfering with C3 CPU state.
	 */
	pci_write_config_byte(PCI_CFG_RETRY_TIMEOUT, 0x00);

	iwl_enable_interrupts(priv);

	if (!(iwl_read32(priv, CSR_GP_CNTRL) &
				CSR_GP_CNTRL_REG_FLAG_HW_RF_KILL_SW))
		hw_rfkill = true;

	if (hw_rfkill)
		set_bit(STATUS_RF_KILL_HW, &priv->status);
	else
		clear_bit(STATUS_RF_KILL_HW, &priv->status);

	wiphy_rfkill_set_hw_state(priv->hw->wiphy, hw_rfkill);

	return 0;
}
