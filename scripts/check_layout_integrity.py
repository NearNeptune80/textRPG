#!/usr/bin/env python3
"""
Layout Integrity and Containment Validator for textRPG
Validates that all UI layout definitions are fully data-driven within JSON configuration,
container sizes strictly add up to 100%, state coverage is complete, and no hardcoded
fullscreen/panel coordinates exist in C++ view implementations.
"""

import json
import os
import sys
import glob
import re

REQUIRED_GAMEPLAY_STATES = [
    "EXPLORATION",
    "COMBAT",
    "SEX",
    "INVENTORY",
    "SHOP",
    "TRANSFORMATION",
    "PHONE_APP",
    "CHARACTER_CREATION",
    "SETTINGS",
    "MAIN_MENU",
    "LOAD_GAME"
]

THREE_COLUMN_GAMEPLAY_STATES = [
    "EXPLORATION",
    "INVENTORY",
    "SHOP",
    "TRANSFORMATION",
    "PHONE_APP",
    "CHARACTER_CREATION",
    "SETTINGS",
    "LOAD_GAME"
]

def validate_node(node, path="root"):
    node_type = node.get("type")
    node_id = node.get("id", "unnamed")
    current_path = f"{path} -> {node_id}"

    if node_type == "CONTAINER":
        children = node.get("children", [])
        sizes = node.get("sizes", [])
        direction = node.get("direction", "")

        if direction not in ("ROW", "COLUMN"):
            print(f"[FAIL] {current_path}: Invalid container direction '{direction}'")
            return False

        if len(children) != len(sizes):
            print(f"[FAIL] {current_path}: Mismatch between children count ({len(children)}) and sizes count ({len(sizes)})")
            return False

        total_size = sum(sizes)
        if abs(total_size - 100.0) > 0.1:
            print(f"[FAIL] {current_path}: Container sizes sum to {total_size:.2f}%, expected 100.0%")
            return False

        for child in children:
            if not validate_node(child, current_path):
                return False

    elif node_type == "LEAF":
        widgets = node.get("widgets", [])
        if not isinstance(widgets, list):
            print(f"[FAIL] {current_path}: Leaf widgets must be an array")
            return False
        for w in widgets:
            if not isinstance(w, str) or not w.strip():
                print(f"[FAIL] {current_path}: Invalid widget entry '{w}'")
                return False
    else:
        print(f"[FAIL] {current_path}: Unknown node type '{node_type}'")
        return False

    return True

def validate_layout_file(filepath):
    print(f"[CHECK] Validating layout file: {filepath}")
    with open(filepath, "r", encoding="utf-8") as f:
        try:
            data = json.load(f)
        except Exception as e:
            print(f"[FAIL] {filepath} is not valid JSON: {e}")
            return False

    # Validate default rootNode
    root = data.get("rootNode")
    if not root or not validate_node(root, "default_root"):
        return False

    # Validate stateOverrides
    overrides = data.get("stateOverrides", {})
    for state_name in REQUIRED_GAMEPLAY_STATES:
        if state_name not in overrides:
            print(f"[FAIL] {filepath}: Missing required stateOverride for '{state_name}'")
            return False

        state_data = overrides[state_name]
        state_root = state_data.get("rootNode")
        if not state_root or not validate_node(state_root, f"stateOverrides[{state_name}]"):
            return False

        # Validate standard 3-column layouts on default_layout.json and custom_tile_layout.json
        if os.path.basename(filepath) in ("default_layout.json", "custom_tile_layout.json") and state_name in THREE_COLUMN_GAMEPLAY_STATES:
            if state_root.get("type") != "CONTAINER" or state_root.get("direction") != "ROW":
                print(f"[FAIL] {filepath}: State '{state_name}' rootNode must be a ROW container")
                return False
            children = state_root.get("children", [])
            if len(children) != 3:
                print(f"[FAIL] {filepath}: State '{state_name}' must have exactly 3 columns (left, center, right), found {len(children)}")
                return False

    print(f"[PASS] {filepath}: All containers, state overrides, and 3-column invariants valid.")
    return True

def audit_cpp_views_for_hardcoding(workspace_dir):
    print("\n[CHECK] Auditing C++ views for layout hardcoding violations...")
    view_files = glob.glob(os.path.join(workspace_dir, "src", "ui", "views", "*.cpp"))
    forbidden_patterns = [
        (re.compile(r'SDL_FRect\s+\w+\s*=\s*\{\s*0(\.0f)?\s*,\s*0(\.0f)?\s*,\s*\d+'), "Hardcoded fullscreen/absolute window origin {0, 0, ...}"),
        (re.compile(r'rect\s*=\s*\{\s*\d+(\.\d+)?f?\s*,\s*\d+(\.\d+)?f?\s*,\s*1[2-9]\d{2}'), "Hardcoded fixed window resolution rect assignment"),
    ]

    violations = 0
    for vf in view_files:
        with open(vf, "r", encoding="utf-8") as f:
            lines = f.readlines()
            for line_idx, line in enumerate(lines, 1):
                for pattern, desc in forbidden_patterns:
                    if pattern.search(line):
                        print(f"[WARN] {vf}:{line_idx}: {desc}: {line.strip()}")
                        violations += 1

    if violations == 0:
        print("[PASS] Zero hardcoded fullscreen panel coordinates found in C++ views.")
        return True
    else:
        print(f"[FAIL] Found {violations} potential layout hardcoding violations.")
        return False

def main():
    workspace_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    layout_files = glob.glob(os.path.join(workspace_dir, "data", "layouts", "*.json"))
    
    if not layout_files:
        print(f"[FAIL] No layout JSON files found in {workspace_dir}/data/layouts/")
        sys.exit(1)

    all_passed = True
    for lf in layout_files:
        if not validate_layout_file(lf):
            all_passed = False

    if not audit_cpp_views_for_hardcoding(workspace_dir):
        all_passed = False

    if all_passed:
        print("\n======================================================================")
        print(" [Layout Integrity Check]: ALL CHECKS PASSED")
        print(" Layout is 100% data-driven, state coverage complete, no hardcoding.")
        print("======================================================================")
        sys.exit(0)
    else:
        print("\n======================================================================")
        print(" [Layout Integrity Check]: INTEGRITY FAILURES DETECTED")
        print("======================================================================")
        sys.exit(1)

if __name__ == "__main__":
    main()
