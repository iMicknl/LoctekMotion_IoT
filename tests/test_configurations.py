"""Behavioral contracts for the ESPHome fixture matrix."""

import subprocess
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class ConfigurationTests(unittest.TestCase):
    """Verify fixtures resolve into the components CI intends to compile."""

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

    def test_passthrough_commands_use_exact_value_routing(self) -> None:
        """Ensure discrete command values are handled outside open ranges."""
        rendered = self.render_config("office-desk-esp32-passthrough.yaml")
        command_config = rendered.split(
            "platform: loctekmotion_desk_command", maxsplit=1
        )[1].split("\nswitch:", maxsplit=1)[0]

        self.assertNotIn("on_value_range:", command_config)
        self.assertIn("on_value:", command_config)
        for value in range(1, 9):
            with self.subTest(value=value):
                self.assertIn(f"return x == {value};", command_config)

    def test_motion_automations_have_bounded_central_control(self) -> None:
        """Require every production package to stop stalled movement."""
        for fixture in (
            "office-desk-esp32.yaml",
            "office-desk-esp32-passthrough.yaml",
        ):
            with self.subTest(fixture=fixture):
                rendered = self.render_config(fixture)
                for script_id in (
                    "start_moving_up",
                    "start_moving_down",
                    "movement_failsafe",
                ):
                    self.assertIn(f"id: {script_id}", rendered)

                self.assertGreaterEqual(rendered.count("timeout: 30s"), 4)
                self.assertIn("has_state()", rendered)
                self.assertIn("std::isfinite", rendered)

                up_switch = rendered.split("id: switch_up", maxsplit=1)[1].split(
                    "id: switch_down", maxsplit=1
                )[0]
                down_switch = rendered.split("id: switch_down", maxsplit=1)[1].split(
                    "id: switch_alarm", maxsplit=1
                )[0]
                self.assertNotIn("on_turn_on:", up_switch)
                self.assertNotIn("on_turn_on:", down_switch)


if __name__ == "__main__":
    unittest.main()
