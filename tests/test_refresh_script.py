import unittest
from pathlib import Path

import yaml


class _SecretsPreservingLoader(yaml.SafeLoader):
    pass


def _construct_secret(loader, node):
    return node.value


_SecretsPreservingLoader.add_constructor("!secret", _construct_secret)

CONFIG_PATH = Path(__file__).resolve().parent.parent / "epaper-panel.yaml"


class RefreshScriptStructureTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with CONFIG_PATH.open("r", encoding="utf-8") as handle:
            cls.config = yaml.load(handle, Loader=_SecretsPreservingLoader)

    def test_refresh_logic_header_included(self):
        includes = self.config.get("esphome", {}).get("includes", [])
        self.assertIn(
            "custom_components/refresh_logic/refresh_logic.h",
            includes,
            "Expected refresh logic helper to be included",
        )

    def test_refresh_action_global_exists(self):
        globals_section = self.config.get("globals", [])
        matching = [item for item in globals_section if item.get("id") == "refresh_action_decision"]
        self.assertTrue(matching, "refresh_action_decision global must be defined")
        entry = matching[0]
        self.assertEqual(entry.get("type"), "int")
        self.assertEqual(entry.get("initial_value"), "0")

    def test_refresh_script_branches(self):
        scripts = self.config.get("script", [])
        refresh_script = None
        for script in scripts:
            if script.get("id") == "refresh_display_and_sleep":
                refresh_script = script
                break
        self.assertIsNotNone(refresh_script, "refresh_display_and_sleep script must exist")

        steps = refresh_script.get("then", [])
        # Ensure the helper lambda populates refresh_action_decision before branching
        helper_lambdas = [
            step
            for step in steps
            if "lambda" in step and "refresh_logic::RefreshContext" in step["lambda"]
        ]
        self.assertTrue(helper_lambdas, "Expected a lambda that uses RefreshContext")

        # Look for the WAIT branch logging message
        wait_branches = [
            step
            for step in steps
            if "if" in step
            and "WAIT_FOR_DATA" in step["if"]["condition"].get("lambda", "")
        ]
        self.assertTrue(wait_branches, "WAIT_FOR_DATA branch must be present")

        def find_branch(node, predicate):
            if isinstance(node, dict):
                if predicate(node):
                    return node
                for value in node.values():
                    result = find_branch(value, predicate)
                    if result is not None:
                        return result
            elif isinstance(node, list):
                for item in node:
                    result = find_branch(item, predicate)
                    if result is not None:
                        return result
            return None

        stay_awake_branch = find_branch(
            steps,
            lambda candidate: "if" in candidate
            and "STAY_AWAKE" in candidate["if"].get("condition", {}).get("lambda", ""),
        )
        self.assertIsNotNone(stay_awake_branch, "Expected STAY_AWAKE conditional branch")
        stay_then = stay_awake_branch["if"].get("then", [])
        delays = [item for item in stay_then if isinstance(item, dict) and item.get("delay") == "2min"]
        self.assertTrue(delays, "Charging branch should delay for 2 minutes before rescheduling")

        deep_else = stay_awake_branch["if"].get("else", []) or []
        self.assertTrue(
            any(isinstance(item, dict) and item.get("delay") == "15s" for item in deep_else),
            "Deep sleep branch should include 15s delay",
        )
        self.assertTrue(
            any(
                isinstance(item, dict) and "deep_sleep.enter" in next(iter(item))
                for item in deep_else
                if isinstance(item, dict)
            ),
            "Deep sleep branch should trigger deep_sleep.enter",
        )


if __name__ == "__main__":
    unittest.main()

