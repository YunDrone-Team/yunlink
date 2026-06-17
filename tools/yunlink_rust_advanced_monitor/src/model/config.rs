use yunlink::AgentType;

/// Launch-time configuration for the monitor.
///
/// The option names mirror the C++ Advanced Monitor so both tools can be used
/// against the same local or dual-host test setups.
#[derive(Debug, Clone)]
pub struct MonitorConfig {
    /// Remote peer IP used by the Connect button.
    pub remote_ip: String,
    /// Remote peer TCP listen port used by the Connect button.
    pub remote_tcp_port: u16,
    /// Local UDP bind port passed into the YunLink runtime.
    pub udp_bind_port: u16,
    /// Local UDP target port passed into the YunLink runtime.
    pub udp_target_port: u16,
    /// Local TCP listen port passed into the YunLink runtime.
    pub tcp_listen_port: u16,
    /// Friendly agent role name such as `ground` or `uav`.
    pub agent_name: String,
    /// Numeric agent id used in the local endpoint identity.
    pub agent_id: u32,
    /// Shared secret forwarded to the safe Rust SDK and then to the C ABI config.
    pub shared_secret: String,
    /// Session node name used when opening a session.
    pub node_name: String,
    /// Authority lease duration requested by the UI.
    pub authority_ttl_ms: u32,
}

impl Default for MonitorConfig {
    fn default() -> Self {
        Self {
            remote_ip: "127.0.0.1".to_string(),
            remote_tcp_port: 9696,
            udp_bind_port: 9797,
            udp_target_port: 9898,
            tcp_listen_port: 9797,
            agent_name: "ground".to_string(),
            agent_id: 7,
            shared_secret: "yunlink-secret".to_string(),
            node_name: "yunlink_rust_advanced_monitor".to_string(),
            authority_ttl_ms: 3000,
        }
    }
}

impl MonitorConfig {
    /// Parse `--key=value` arguments without adding a CLI dependency.
    pub fn from_args(args: impl Iterator<Item = String>) -> Self {
        let mut config = Self::default();
        for arg in args {
            let Some((key, value)) = arg.split_once('=') else {
                continue;
            };
            match key {
                "--remote-ip" => config.remote_ip = value.to_string(),
                "--remote-tcp-port" => {
                    config.remote_tcp_port = parse_u16(value, config.remote_tcp_port)
                }
                "--udp-bind-port" => config.udp_bind_port = parse_u16(value, config.udp_bind_port),
                "--udp-target-port" => {
                    config.udp_target_port = parse_u16(value, config.udp_target_port)
                }
                "--tcp-listen-port" => {
                    config.tcp_listen_port = parse_u16(value, config.tcp_listen_port)
                }
                "--agent-name" => config.agent_name = value.to_string(),
                "--agent-id" => config.agent_id = value.parse().unwrap_or(config.agent_id),
                "--shared-secret" => config.shared_secret = value.to_string(),
                "--node-name" => config.node_name = value.to_string(),
                "--authority-ttl-ms" => {
                    config.authority_ttl_ms = value.parse().unwrap_or(config.authority_ttl_ms)
                }
                _ => {}
            }
        }
        config
    }

    /// Convert the friendly CLI role name into the SDK enum used by RuntimeConfig.
    pub fn agent_type(&self) -> AgentType {
        match self.agent_name.as_str() {
            "uav" | "air" => AgentType::Uav,
            "ugv" => AgentType::Ugv,
            "swarm" | "swarm-controller" => AgentType::SwarmController,
            _ => AgentType::GroundStation,
        }
    }
}

/// Parse a u16 CLI value, preserving a known-good fallback on malformed input.
fn parse_u16(value: &str, fallback: u16) -> u16 {
    value.parse::<u16>().unwrap_or(fallback)
}

#[cfg(test)]
mod tests {
    use super::MonitorConfig;

    #[test]
    fn cli_args_override_monitor_config() {
        let config = MonitorConfig::from_args(
            [
                "--remote-ip=10.0.0.5",
                "--remote-tcp-port=1234",
                "--udp-bind-port=1235",
                "--udp-target-port=1236",
                "--tcp-listen-port=1237",
                "--agent-name=uav",
                "--agent-id=9",
                "--shared-secret=secret",
                "--node-name=rust-monitor",
                "--authority-ttl-ms=4500",
            ]
            .into_iter()
            .map(String::from),
        );

        assert_eq!(config.remote_ip, "10.0.0.5");
        assert_eq!(config.remote_tcp_port, 1234);
        assert_eq!(config.udp_bind_port, 1235);
        assert_eq!(config.udp_target_port, 1236);
        assert_eq!(config.tcp_listen_port, 1237);
        assert_eq!(config.agent_name, "uav");
        assert_eq!(config.agent_id, 9);
        assert_eq!(config.shared_secret, "secret");
        assert_eq!(config.node_name, "rust-monitor");
        assert_eq!(config.authority_ttl_ms, 4500);
    }
}
