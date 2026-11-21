# 기술 설계와 빌드

프로젝트 루트의 testset_6_div_max.xlsx가 기본 입력이다. Sheet1 첫 행의 266.0nm~1000.0nm 열 500개를 파장순으로 읽고 Material 열로 정답을 연결한다. Shape·반경·시간 등 조건 열은 그래프에서 제외한다. 각 물질의 첫 고유 스펙트럼 4개를 선정하고 정해진 순서로 배열한다. 원본 시트명과 행 번호를 데이터와 함께 내장한다. 기존 물질별 시트 형식도 명시적인 파일 경로로 빌드할 수 있다.

## 제작 환경

Linux 제작 머신에 Python 3, python3-venv, g++-mingw-w64-x86-64 및 binutils-mingw-w64-x86-64를 준비한다. 저장소 루트에서 실행한다.

```sh
python3 -m venv /tmp/material-env
/tmp/material-env/bin/pip install openpyxl
PYTHON=/tmp/material-env/bin/python ./build.sh
```

다른 입력은 마지막 명령 뒤에 .xlsx 경로를 추가한다. 데이터 검증에 실패하면 기존 배포 파일을 보존한다. 성공하면 dist/MaterialClassifier.exe에 데이터와 팔레트가 포함된다.

## 화면과 검증

Win32 버튼은 마우스 및 키보드 입력을 처리하고, GDI+가 보드와 상세 화면에 동일 레코드의 곡선을 그린다. 선의 세로 위치에 따라 색상을 연속 보간한다. X축은 200–1000 nm로 고정하고 266 nm에서 첫 측정점을 그린다. 기본 덱의 Y축은 0–1이며 추가 로그 변환은 적용하지 않는다. 다른 파일은 카드의 세기 범위에 0을 포함해 표시한다. 피크가 동률이면 가장 짧은 파장에 수선을 그린다. Windows 기본 그래픽 DLL을 사용한다.

개발 검증 명령은 AGENTS.md를 따른다. 원본과의 수치 일치 및 논리 검증, Wine 실행, 실제 행사 PC 검수를 구별한다.
