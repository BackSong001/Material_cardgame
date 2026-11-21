# 기술 설계와 빌드

C++17, Win32 대화상자와 키보드 조작이 가능한 버튼, GDI 선 그래프를 사용한다. 한 카드의 물질, 원본 시트·행, X/Y 배열을 함께 보관하고 미리보기와 상세 화면이 같은 그리기 함수를 호출한다. 셔플 순열은 한 판 동안 고정한다. 선택 위치, 표시 중인 면, 각 위치의 확인 여부를 메모리에 둔다.

## 제작 환경

Linux에 g++-mingw-w64-x86-64 및 binutils-mingw-w64-x86-64와 Python 3을 준비한다. 기본 빌드는 ./build.sh 이며 data/demo.csv의 모의 덱을 사용한다. 실측 파일은 다음과 같이 지정한다.

```sh
python3 -m venv /tmp/material-env
/tmp/material-env/bin/pip install openpyxl
PYTHON=/tmp/material-env/bin/python ./build.sh '/path/to/spectra.xlsx'
```

시트명은 Cu, W, Ti, SiO2, Si3N4이다. 화학식 아래첨자도 인식한다. 각 시트의 첫 행 전체에 숫자 파장(nm)을, 둘째 행부터 샘플당 한 줄씩 같은 열에 숫자 세기를 입력한다. 파장 열은 최소 두 개이며 값이 고유해야 한다. 수식은 저장된 계산값이 필요하다. 별도 ID 열은 두지 않는다.

변환기는 파장과 대응 세기를 함께 정렬하고, 빈 행을 건너뛴 뒤 각 시트에서 먼저 나타나는 고유 샘플 4개를 선택한다. 시트 간 동일 스펙트럼이나 결측값을 발견하면 위치를 포함한 오류로 빌드를 중단한다. 원본 행 번호는 1부터 센다. 지정한 파일이 실패해도 기존 EXE는 보존한다.

색상과 데이터는 임시 C++ include 파일을 거쳐 정적 링크한 dist/MaterialClassifier.exe에 내장된다. Windows 실행 시 Python, Excel, 엑셀 원본, 색상 파일이 필요 없다. WINE에서 동작과 DLL 의존성을 검사하고 실제 오프라인 Windows 검수는 별도로 기록한다.
