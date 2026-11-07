//! Per-node link quality: RSSI, SNR, packet loss. Fixed-point, no heap.
//!
//! RSSI and SNR are carried as `i16` in hundredths of a unit (centi-dBm and
//! centi-dB), so `-87.25 dBm` is `-8725`. Loss is derived from gaps in the
//! sender's 16-bit sequence number, with wraparound.

use crate::packet::NodeId;

/// How far back (in sequence numbers) a late packet still counts as
/// "reordered". Anything further back is treated as a sender restart.
pub const REORDER_WINDOW: u16 = 64;

/// The running average moves 1/8 of the way to each new sample (integer
/// arithmetic, so it settles within a few hundredths of a unit).
const EMA_DIVISOR: i32 = 8;

/// Last, min, max and exponential average of one measured quantity.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Metric {
    pub last: i16,
    pub min: i16,
    pub max: i16,
    pub avg: i16,
}

impl Metric {
    pub const fn new(v: i16) -> Self {
        Self {
            last: v,
            min: v,
            max: v,
            avg: v,
        }
    }

    pub fn update(&mut self, v: i16) {
        self.last = v;
        if v < self.min {
            self.min = v;
        }
        if v > self.max {
            self.max = v;
        }
        let a = i32::from(self.avg);
        // avg always stays between min and max, so it fits in i16
        self.avg = (a + (i32::from(v) - a) / EMA_DIVISOR) as i16;
    }
}

/// What a received sequence number meant for the loss accounting.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Outcome {
    /// First packet from this node.
    New,
    /// The expected next packet (`lost == 0`) or one after a gap of `lost` packets.
    InOrder { lost: u16 },
    /// Same sequence number as the last packet; ignored.
    Duplicate,
    /// A late packet inside [`REORDER_WINDOW`]; it is no longer counted as lost.
    Reordered,
    /// The sequence number jumped back further than the window: sender restarted.
    Restarted,
}

/// Link statistics for one remote node.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct NodeStats {
    pub node: NodeId,
    pub rx_count: u32,
    pub lost_count: u32,
    pub reordered: u16,
    pub restarts: u16,
    pub last_seq: u16,
    pub last_seen_ms: u32,
    /// Received signal strength in centi-dBm.
    pub rssi: Metric,
    /// Signal-to-noise ratio in centi-dB.
    pub snr: Metric,
}

impl NodeStats {
    pub const fn new(node: NodeId, seq: u16, rssi: i16, snr: i16, now_ms: u32) -> Self {
        Self {
            node,
            rx_count: 1,
            lost_count: 0,
            reordered: 0,
            restarts: 0,
            last_seq: seq,
            last_seen_ms: now_ms,
            rssi: Metric::new(rssi),
            snr: Metric::new(snr),
        }
    }

    /// Records one received packet from this node.
    pub fn record(&mut self, seq: u16, rssi: i16, snr: i16, now_ms: u32) -> Outcome {
        let delta = seq.wrapping_sub(self.last_seq);
        if delta == 0 {
            self.last_seen_ms = now_ms;
            return Outcome::Duplicate;
        }

        let outcome = if delta < 0x8000 {
            // forward: delta - 1 packets in between were never heard
            let lost = delta - 1;
            self.lost_count = self.lost_count.saturating_add(u32::from(lost));
            self.last_seq = seq;
            Outcome::InOrder { lost }
        } else {
            let back = self.last_seq.wrapping_sub(seq);
            if back <= REORDER_WINDOW {
                self.reordered = self.reordered.saturating_add(1);
                self.lost_count = self.lost_count.saturating_sub(1);
                Outcome::Reordered
            } else {
                self.restarts = self.restarts.saturating_add(1);
                self.last_seq = seq;
                Outcome::Restarted
            }
        };

        self.rx_count = self.rx_count.saturating_add(1);
        self.last_seen_ms = now_ms;
        self.rssi.update(rssi);
        self.snr.update(snr);
        outcome
    }

    /// Packet loss in permille (0..=1000).
    pub fn loss_permille(&self) -> u16 {
        let total = u64::from(self.rx_count) + u64::from(self.lost_count);
        if total == 0 {
            return 0;
        }
        (u64::from(self.lost_count) * 1000 / total) as u16
    }
}

/// The node table is full and the node is not yet known.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct TableFull;

/// Fixed-capacity registry of remote nodes.
#[derive(Clone, Debug)]
pub struct NodeTable<const N: usize> {
    slots: [Option<NodeStats>; N],
}

impl<const N: usize> NodeTable<N> {
    pub const fn new() -> Self {
        Self { slots: [None; N] }
    }

    /// Records a packet from `node`, adding the node if there is room.
    pub fn record(
        &mut self,
        node: NodeId,
        seq: u16,
        rssi: i16,
        snr: i16,
        now_ms: u32,
    ) -> Result<Outcome, TableFull> {
        if let Some(s) = self.slots.iter_mut().flatten().find(|s| s.node == node) {
            return Ok(s.record(seq, rssi, snr, now_ms));
        }
        match self.slots.iter_mut().find(|s| s.is_none()) {
            Some(slot) => {
                *slot = Some(NodeStats::new(node, seq, rssi, snr, now_ms));
                Ok(Outcome::New)
            }
            None => Err(TableFull),
        }
    }

    pub fn get(&self, node: NodeId) -> Option<&NodeStats> {
        self.slots.iter().flatten().find(|s| s.node == node)
    }

    pub fn len(&self) -> usize {
        self.slots.iter().flatten().count()
    }

    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    pub fn iter(&self) -> impl Iterator<Item = &NodeStats> {
        self.slots.iter().flatten()
    }
}

impl<const N: usize> Default for NodeTable<N> {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn fresh(seq: u16) -> NodeStats {
        NodeStats::new(1, seq, -9000, 800, 0)
    }

    #[test]
    fn consecutive_packets_have_no_loss() {
        let mut s = fresh(10);
        for seq in 11..=20 {
            assert_eq!(s.record(seq, -9000, 800, 0), Outcome::InOrder { lost: 0 });
        }
        assert_eq!(s.rx_count, 11);
        assert_eq!(s.lost_count, 0);
        assert_eq!(s.loss_permille(), 0);
    }

    #[test]
    fn gap_counts_lost_packets() {
        let mut s = fresh(10);
        assert_eq!(s.record(14, -9000, 800, 0), Outcome::InOrder { lost: 3 });
        assert_eq!(s.lost_count, 3);
        assert_eq!(s.rx_count, 2);
        assert_eq!(s.loss_permille(), 600); // 3 lost / (2 received + 3 lost)
    }

    #[test]
    fn wraparound_without_gap_has_no_loss() {
        let mut s = fresh(u16::MAX);
        assert_eq!(s.record(0, -9000, 800, 0), Outcome::InOrder { lost: 0 });
        assert_eq!(s.lost_count, 0);
    }

    #[test]
    fn wraparound_with_gap_counts_loss() {
        let mut s = fresh(u16::MAX - 1);
        assert_eq!(s.record(1, -9000, 800, 0), Outcome::InOrder { lost: 2 });
        assert_eq!(s.lost_count, 2);
    }

    #[test]
    fn duplicate_is_ignored() {
        let mut s = fresh(5);
        assert_eq!(s.record(5, -1, 1, 99), Outcome::Duplicate);
        assert_eq!(s.rx_count, 1);
        assert_eq!(s.last_seen_ms, 99);
        assert_eq!(s.rssi.last, -9000); // metrics untouched
    }

    #[test]
    fn late_packet_is_reordered_and_no_longer_lost() {
        let mut s = fresh(10);
        assert_eq!(s.record(13, -9000, 800, 0), Outcome::InOrder { lost: 2 });
        assert_eq!(s.lost_count, 2);
        assert_eq!(s.record(12, -9000, 800, 0), Outcome::Reordered);
        assert_eq!(s.lost_count, 1);
        assert_eq!(s.reordered, 1);
        assert_eq!(s.last_seq, 13);
        assert_eq!(s.rx_count, 3);
    }

    #[test]
    fn reorder_never_underflows_lost_count() {
        let mut s = fresh(10);
        assert_eq!(s.record(5, -9000, 800, 0), Outcome::Reordered);
        assert_eq!(s.lost_count, 0);
    }

    #[test]
    fn big_backward_jump_is_a_restart() {
        let mut s = fresh(1000);
        assert_eq!(s.record(3, -9000, 800, 0), Outcome::Restarted);
        assert_eq!(s.restarts, 1);
        assert_eq!(s.last_seq, 3);
        assert_eq!(s.lost_count, 0);
        // and counting continues normally afterwards
        assert_eq!(s.record(4, -9000, 800, 0), Outcome::InOrder { lost: 0 });
    }

    #[test]
    fn window_boundary() {
        let mut s = fresh(1000);
        assert_eq!(s.record(1000 - REORDER_WINDOW, 0, 0, 0), Outcome::Reordered);
        let mut s = fresh(1000);
        assert_eq!(
            s.record(1000 - REORDER_WINDOW - 1, 0, 0, 0),
            Outcome::Restarted
        );
    }

    #[test]
    fn metrics_track_min_max_last_and_converge() {
        let mut m = Metric::new(-9000);
        m.update(-8000);
        m.update(-9500);
        assert_eq!((m.last, m.min, m.max), (-9500, -9500, -8000));
        assert!(m.avg >= m.min && m.avg <= m.max);

        let mut m = Metric::new(0);
        for _ in 0..200 {
            m.update(1000);
        }
        assert!(
            (m.avg - 1000).abs() <= 8,
            "avg {} should settle near 1000",
            m.avg
        );
    }

    #[test]
    fn metrics_hold_realistic_extremes() {
        let mut m = Metric::new(-15000); // -150 dBm
        m.update(i16::MAX);
        m.update(i16::MIN);
        assert_eq!((m.min, m.max), (i16::MIN, i16::MAX));
    }

    #[test]
    fn table_adds_updates_and_reports_full() {
        let mut t: NodeTable<2> = NodeTable::new();
        assert!(t.is_empty());
        assert_eq!(t.record(1, 0, -9000, 800, 0), Ok(Outcome::New));
        assert_eq!(t.record(2, 0, -9000, 800, 0), Ok(Outcome::New));
        assert_eq!(
            t.record(1, 1, -9000, 800, 5),
            Ok(Outcome::InOrder { lost: 0 })
        );
        assert_eq!(t.record(3, 0, -9000, 800, 0), Err(TableFull));
        assert_eq!(t.len(), 2);
        assert_eq!(t.get(1).unwrap().rx_count, 2);
        assert!(t.get(3).is_none());
        assert_eq!(t.iter().count(), 2);
    }

    #[test]
    fn loss_permille_is_zero_with_no_traffic_and_bounded() {
        let mut s = fresh(0);
        s.rx_count = 0;
        assert_eq!(s.loss_permille(), 0);
        s.lost_count = u32::MAX;
        s.rx_count = 1;
        assert!(s.loss_permille() <= 1000);
    }
}
