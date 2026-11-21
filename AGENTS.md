# 개발 규칙

- 최종 덱은 Cu, W, Al, SiO₂, Si₃N₄ 각 4장이다. 파장과 세기 쌍, 정답, 출처 행을 한 레코드에 유지한다.
- 보드 위치를 실행 간 고정하고 복귀 시 포커스를 복원한다.
- 기본 덱은 루트의 testset_5_x100.xlsx에서 생성한다. 색상 변경은 colors.json과 DISGIN.md에 함께 반영한다.
- 다음 명령으로 덱, 진행 상태, 엑셀 입력 검증, 배포 빌드를 확인한다. 모든 명령은 openpyxl 설치 환경에서 실행한다.

```sh
tmp=$(mktemp -d)
python3 tools/prepare.py "$tmp"
g++ -std=c++17 -I"$tmp" tests/game.cpp -o "$tmp/test" && "$tmp/test"
python3 tests/workbook.py
./build.sh
```
