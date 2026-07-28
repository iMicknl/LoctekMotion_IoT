"""Behavioral contracts for the ESPHome fixture matrix."""

import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class ConfigurationTests(unittest.TestCase):
    """Verify fixtures resolve into the components CI intends to compile."""

    def test_native_protocol_decoders(self) -> None:
        """Compile and run the allocation-free protocol decoder tests."""
        with tempfile.TemporaryDirectory() as temp_dir:
            for source_name in (
                "height_decoder_test.cpp",
                "command_decoder_test.cpp",
            ):
                with self.subTest(source=source_name):
                    source = ROOT / "tests" / source_name
                    binary = Path(temp_dir) / source.stem
                    compile_result = subprocess.run(
                        [
                            "c++",
                            "-std=c++17",
                            "-Wall",
                            "-Wextra",
                            "-Werror",
                            "-pedantic",
                            f"-I{ROOT}",
                            str(source),
                            "-o",
                            str(binary),
                        ],
                        cwd=ROOT,
                        capture_output=True,
                        check=False,
                        text=True,
                    )
                    self.assertEqual(
                        compile_result.returncode,
                        0,
                        msg=(
                            f"{source_name} failed compilation:\n"
                            f"{compile_result.stdout}\n{compile_result.stderr}"
                        ),
                    )

                    run_result = subprocess.run(
                        [str(binary)],
                        cwd=ROOT,
                        capture_output=True,
                        check=False,
                        text=True,
                    )
                    self.assertEqual(
                        run_result.returncode,
                        0,
                        msg=(
                            f"{source_name} failed:\n"
                            f"{run_result.stdout}\n{run_result.stderr}"
                        ),
                    )

    def render_config(self, fixture: str) -> str:
        """Return ESPHome's fully resolved configuration for a fixture."""
        result = subprocess.run(
            ["esphome", "config", str(ROOT / "tests" / fixture)],
            cwd=ROOT,
            capture_output=True,
            check=False,
            text=True,
        )
        self.assertEqual(
            result.returncode,
            0,
            msg=f"{fixture} failed validation:\n{result.stdout}\n{result.stderr}",
        )
        return result.stdout

    def test_passthrough_fixtures_exercise_command_component(self) -> None:
        """Keep pass-through CI from silently compiling empty configurations."""
        for fixture in (
            "office-desk-esp32-passthrough.yaml",
            "office-desk-esp8266-passthrough.yaml",
        ):
            with self.subTest(fixture=fixture):
                rendered = self.render_config(fixture)
                self.assertIn(
                    "platform: loctekmotion_desk_command",
                    rendered,
                    msg=f"{fixture} does not exercise the command component",
                )

    def test_passthrough_routes_exact_command_values(self) -> None:
        """Require exact keypad dispatch, including the second payload byte."""
        rendered = self.render_config("office-desk-esp32-passthrough.yaml")
        start = rendered.index("platform: loctekmotion_desk_command")
        end = rendered.index("\nswitch:", start)
        command_sensor = rendered[start:end]

        self.assertIn("on_value:", command_sensor)
        self.assertNotIn("on_value_range:", command_sensor)
        for value in range(1, 10):
            with self.subTest(value=value):
                self.assertIn(f"return x == {value};", command_sensor)

    def test_published_packages_bound_motion(self) -> None:
        """Keep all software-started motion cancellable and time-bounded."""
        for fixture in (
            "office-desk-esp32.yaml",
            "office-desk-esp32-passthrough.yaml",
        ):
            with self.subTest(fixture=fixture):
                rendered = self.render_config(fixture)
                self.assertIn("movement_timeout: 30s", rendered)
                self.assertIn("id: start_moving_up", rendered)
                self.assertIn("id: start_moving_down", rendered)
                self.assertIn("id: movement_failsafe", rendered)
                self.assertGreaterEqual(rendered.count("timeout: 30s"), 4)
                self.assertIn("id(desk_height).has_state()", rendered)
                self.assertIn("std::isfinite(id(desk_height).state)", rendered)
                stop_start = rendered.index("stop_action:")
                stop_end = rendered.index("\n    open_action:", stop_start)
                stop_action = rendered[stop_start:stop_end]
                self.assertIn("id: start_moving_up", stop_action)
                self.assertIn("id: start_moving_down", stop_action)
                self.assertIn("id: movement_failsafe", stop_action)


if __name__ == "__main__":
    unittest.main()
