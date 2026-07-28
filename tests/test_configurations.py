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


if __name__ == "__main__":
    unittest.main()
