#!/usr/bin/env python3
"""
MacType INI to JSON 변환 도구
기존 INI 설정 파일을 JSON 형식으로 변환합니다.
"""

import sys
import os
import json
import configparser
from pathlib import Path

def convert_ini_to_json(ini_path, json_path):
    """INI 파일을 JSON으로 변환"""
    config = configparser.ConfigParser()
    config.optionxform = str  # 대소문자 유지

    try:
        # INI 파일 읽기 (UTF-8 with BOM 지원)
        config.read(ini_path, encoding='utf-8-sig')

        # JSON으로 변환
        json_data = {}
        for section in config.sections():
            json_data[section] = dict(config[section])

        # JSON 파일 쓰기
        with open(json_path, 'w', encoding='utf-8') as f:
            json.dump(json_data, f, indent=2, ensure_ascii=False)

        print(f"✅ 변환 완료: {ini_path} -> {json_path}")
        return True

    except Exception as e:
        print(f"❌ 변환 실패: {ini_path}")
        print(f"   오류: {e}")
        return False

def main():
    if len(sys.argv) != 3:
        print("사용법: python convert-ini-to-json.py <input.ini> <output.json>")
        sys.exit(1)

    ini_file = sys.argv[1]
    json_file = sys.argv[2]

    if not os.path.exists(ini_file):
        print(f"❌ INI 파일을 찾을 수 없습니다: {ini_file}")
        sys.exit(1)

    success = convert_ini_to_json(ini_file, json_file)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
