use lama_core::state::State;
use lama_sim::{Link, Network};

const AIRTIME_MS: u32 = 60;

/// Intervals chosen so that no two beacons overlap in the first 10 seconds.
fn two_node_net() -> (Network, usize, usize) {
    let mut net = Network::new(1, AIRTIME_MS);
    let a = net.add_node(Network::config(1, 1000));
    let b = net.add_node(Network::config(2, 1300));
    net.link_both(a, b, Link::GOOD);
    (net, a, b)
}

#[test]
fn nodes_boot_into_idle() {
    let (mut net, a, b) = two_node_net();
    net.run_ms(10);
    assert_eq!(net.node(a).state(), State::Idle);
    assert_eq!(net.node(b).state(), State::Idle);
}

#[test]
fn two_nodes_hear_each_other_without_loss() {
    let (mut net, a, b) = two_node_net();
    net.run_ms(10_000);

    let a_sees_b = net.node(a).table().get(2).expect("a should hear b");
    let b_sees_a = net.node(b).table().get(1).expect("b should hear a");

    assert!(b_sees_a.rx_count >= 8, "b heard only {}", b_sees_a.rx_count);
    assert!(a_sees_b.rx_count >= 6, "a heard only {}", a_sees_b.rx_count);
    assert_eq!(b_sees_a.lost_count, 0);
    assert_eq!(a_sees_b.lost_count, 0);
    assert_eq!(b_sees_a.rssi.last, Link::GOOD.rssi_cdbm);
    assert_eq!(b_sees_a.snr.last, Link::GOOD.snr_cdb);
    assert_eq!(net.node(a).rx_dropped(), 0);
}

#[test]
fn a_node_never_records_itself() {
    let (mut net, a, b) = two_node_net();
    net.run_ms(10_000);
    assert!(net.node(a).table().get(1).is_none());
    assert!(net.node(b).table().get(2).is_none());
}

#[test]
fn measured_loss_matches_configured_link_loss() {
    // Only a -> b exists, and b beacons rarely, so a's frames are almost never
    // blocked by b transmitting. Loss should therefore track the link: 30 %.
    let mut net = Network::new(42, AIRTIME_MS);
    let a = net.add_node(Network::config(1, 1000));
    let b = net.add_node(Network::config(2, 100_000));
    net.link(
        a,
        b,
        Link {
            loss_permille: 300,
            ..Link::GOOD
        },
    );
    net.run_ms(300_000);

    let s = net.node(b).table().get(1).unwrap();
    let loss = s.loss_permille();
    assert!(
        (200..=400).contains(&loss),
        "expected ~300 permille loss, measured {loss}"
    );
    assert!(s.rx_count > 150);
}

#[test]
fn synchronised_beacons_collide_and_nothing_gets_through() {
    // a and b beacon at exactly the same moments; c, in range of both, hears nothing.
    let mut net = Network::new(1, AIRTIME_MS);
    let a = net.add_node(Network::config(1, 1000));
    let b = net.add_node(Network::config(2, 1000));
    let c = net.add_node(Network::config(3, 100_000));
    net.link(a, c, Link::GOOD);
    net.link(b, c, Link::GOOD);
    net.run_ms(10_000);

    assert!(net.node(a).tx_count() >= 9, "a must keep transmitting");
    assert!(net.node(b).tx_count() >= 9, "b must keep transmitting");
    assert!(net.node(c).table().is_empty(), "every frame should collide");
}

#[test]
fn staggered_beacons_are_both_heard() {
    let mut net = Network::new(1, AIRTIME_MS);
    let a = net.add_node(Network::config(1, 1000));
    let b = net.add_node(Network::config(2, 1300));
    let c = net.add_node(Network::config(3, 100_000));
    net.link(a, c, Link::GOOD);
    net.link(b, c, Link::GOOD);
    net.run_ms(10_000);

    for id in [1, 2] {
        let s = net.node(c).table().get(id).expect("c should hear both");
        assert!(s.rx_count >= 6);
        assert_eq!(s.lost_count, 0);
    }
}

#[test]
fn node_out_of_range_is_not_heard() {
    let mut net = Network::new(1, AIRTIME_MS);
    let a = net.add_node(Network::config(1, 1000));
    let b = net.add_node(Network::config(2, 1300));
    let _ = (a, b); // no links at all
    net.run_ms(5_000);
    assert!(net.node(a).table().is_empty());
    assert!(net.node(b).table().is_empty());
}

#[test]
fn radio_init_failures_are_retried_and_recover() {
    let mut net = Network::new(1, AIRTIME_MS);
    let a = net.add_node(Network::config(1, 1000));
    let b = net.add_node(Network::config(2, 1300));
    net.link_both(a, b, Link::GOOD);
    net.fail_begin(a, 2); // two failures, then it works (retry delay 500 ms)

    net.run_ms(300);
    assert_eq!(net.node(a).state(), State::Error);

    net.run_ms(2_000);
    assert_eq!(net.node(a).state(), State::Idle);

    net.run_ms(5_000);
    assert!(
        net.node(b).table().get(1).is_some(),
        "a must be heard after recovering"
    );
}

#[test]
fn persistent_radio_failure_ends_in_fatal_and_stays_silent() {
    let mut net = Network::new(1, AIRTIME_MS);
    let a = net.add_node(Network::config(1, 1000));
    net.fail_begin(a, 100);
    net.run_ms(10_000);
    assert_eq!(net.node(a).state(), State::Fatal);
    assert_eq!(net.node(a).tx_count(), 0);
}

#[test]
fn same_seed_gives_identical_results() {
    let run = |seed| {
        let mut net = Network::new(seed, AIRTIME_MS);
        let a = net.add_node(Network::config(1, 1000));
        let b = net.add_node(Network::config(2, 100_000));
        net.link(
            a,
            b,
            Link {
                loss_permille: 300,
                ..Link::GOOD
            },
        );
        net.run_ms(60_000);
        let s = *net.node(b).table().get(1).unwrap();
        (s.rx_count, s.lost_count, s.last_seq)
    };
    assert_eq!(run(7), run(7));
    assert_ne!(run(7), run(8), "different seeds should differ");
}
