#ifndef __HAILO15_DPHY_H
#define __HAILO15_DPHY_H

#include <linux/phy/phy.h>

int hailo15_dphy_rx_init(struct phy *phy, u64 link_freq);

#endif /*__HAILO15_DPHY_H*/
