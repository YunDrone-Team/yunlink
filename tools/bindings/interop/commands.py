from __future__ import annotations

from .paths import BUILD_DIR, PYTHON_BIN, RUST_EXAMPLE_DIR, scenario_path


def cxx_air_cmd(udp_bind: int, udp_target: int, tcp_listen: int) -> list[str]:
    return [
        str(BUILD_DIR / "example_cxx_air_roundtrip"),
        str(udp_bind),
        str(udp_target),
        str(tcp_listen),
    ]


def rust_ground_cmd(
    ip: str, port: int, udp_bind: int, udp_target: int, tcp_listen: int
) -> list[str]:
    return [
        str(RUST_EXAMPLE_DIR / "ground_roundtrip"),
        ip,
        str(port),
        str(udp_bind),
        str(udp_target),
        str(tcp_listen),
    ]


def rust_ground_recovery_cmd(
    ip: str, port: int, udp_bind: int, udp_target: int, tcp_listen: int, rounds: int = 2
) -> list[str]:
    return [
        str(RUST_EXAMPLE_DIR / "ground_recovery"),
        ip,
        str(port),
        str(udp_bind),
        str(udp_target),
        str(tcp_listen),
        str(rounds),
    ]


def rust_air_cmd(udp_bind: int, udp_target: int, tcp_listen: int) -> list[str]:
    return [
        str(RUST_EXAMPLE_DIR / "air_roundtrip"),
        str(udp_bind),
        str(udp_target),
        str(tcp_listen),
    ]


def python_ground_cmd(
    ip: str, port: int, udp_bind: int, udp_target: int, tcp_listen: int
) -> list[str]:
    return [
        str(PYTHON_BIN),
        str(scenario_path("ground", "python_ground_roundtrip.py")),
        ip,
        str(port),
        str(udp_bind),
        str(udp_target),
        str(tcp_listen),
    ]


def python_air_cmd(udp_bind: int, udp_target: int, tcp_listen: int) -> list[str]:
    return [
        str(PYTHON_BIN),
        str(scenario_path("air", "python_air_roundtrip.py")),
        str(udp_bind),
        str(udp_target),
        str(tcp_listen),
    ]


def python_air_recovery_cmd(udp_bind: int, udp_target: int, tcp_listen: int) -> list[str]:
    return [
        str(PYTHON_BIN),
        str(scenario_path("air", "python_air_recovery.py")),
        str(udp_bind),
        str(udp_target),
        str(tcp_listen),
        "--rounds",
        "2",
    ]


def python_air_restart_cmd(udp_bind: int, udp_target: int, tcp_listen: int) -> list[str]:
    return [
        str(PYTHON_BIN),
        str(scenario_path("air", "python_air_restart.py")),
        str(udp_bind),
        str(udp_target),
        str(tcp_listen),
        "--rounds",
        "2",
    ]


def python_air_competition_cmd(udp_bind: int, udp_target: int, tcp_listen: int) -> list[str]:
    return [
        str(PYTHON_BIN),
        str(scenario_path("air", "python_air_competition.py")),
        str(udp_bind),
        str(udp_target),
        str(tcp_listen),
    ]


def python_ground_competition_cmd(
    ip: str,
    port: int,
    ground_a_udp_bind: int,
    ground_a_udp_target: int,
    ground_a_tcp_listen: int,
    ground_b_udp_bind: int,
    ground_b_udp_target: int,
    ground_b_tcp_listen: int,
) -> list[str]:
    return [
        str(PYTHON_BIN),
        str(scenario_path("ground", "python_ground_competition.py")),
        ip,
        str(port),
        str(ground_a_udp_bind),
        str(ground_a_udp_target),
        str(ground_a_tcp_listen),
        str(ground_b_udp_bind),
        str(ground_b_udp_target),
        str(ground_b_tcp_listen),
    ]


def python_air_dual_uav_cmd(
    uav1_udp_bind: int,
    uav1_udp_target: int,
    uav1_tcp_listen: int,
    uav2_udp_bind: int,
    uav2_udp_target: int,
    uav2_tcp_listen: int,
) -> list[str]:
    return [
        str(PYTHON_BIN),
        str(scenario_path("air", "python_air_dual_uav.py")),
        str(uav1_udp_bind),
        str(uav1_udp_target),
        str(uav1_tcp_listen),
        str(uav2_udp_bind),
        str(uav2_udp_target),
        str(uav2_tcp_listen),
    ]


def python_ground_dual_uav_cmd(
    ip: str,
    uav1_port: int,
    uav2_port: int,
    ground_udp_bind: int,
    ground_udp_target: int,
    ground_tcp_listen: int,
) -> list[str]:
    return [
        str(PYTHON_BIN),
        str(scenario_path("ground", "python_ground_dual_uav.py")),
        ip,
        str(uav1_port),
        str(uav2_port),
        str(ground_udp_bind),
        str(ground_udp_target),
        str(ground_tcp_listen),
    ]
