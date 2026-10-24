# FogHunt — 1단계 설계 (UE 5.6, C++)

비대칭 1 vs 4 호러 프로토타입. 1단계 목표는 **살인마/생존자 이동 + 발전기 수리 + 스킬 체크**를
멀티플레이(서버 권한) 구조로 동작시키는 것.

> 프로젝트 코드네임 `FogHunt`, 클래스 접두사 `FH`. 원작 이름/에셋은 쓰지 않는다.

---

## 1. 폴더 구조

```
FogHunt/
├── FogHunt.uproject
├── Config/
│   ├── DefaultEngine.ini      # 기본 GameMode = FHGameMode
│   ├── DefaultInput.ini       # Enhanced Input 활성화
│   └── DefaultGame.ini
├── Content/                   # 에디터에서 만드는 에셋 (아래 4절 참고)
└── Source/
    ├── FogHunt.Target.cs / FogHuntEditor.Target.cs
    └── FogHunt/
        ├── FogHunt.Build.cs
        ├── FHTypes.h                       # EFHRole, EFHSkillCheckResult
        ├── Core/
        │   ├── FHGameMode                  # 역할 배정, 스폰 위치, 발전기 완료 처리 (서버 전용)
        │   ├── FHGameState                 # 남은 발전기 수, 출구 전원 (모두에게 복제)
        │   └── FHPlayerState               # 플레이어 역할 (복제)
        ├── Characters/
        │   ├── FHCharacterBase             # 공통: 이동/시점/상호작용/입력
        │   ├── FHCharacterMovementComponent# 예측 가능한 달리기 + 상호작용 중 이동 잠금
        │   ├── FHSurvivorCharacter         # 3인칭, 걷기/달리기/앉기, 스킬 체크
        │   └── FHKillerCharacter           # 1인칭, 4.6 m/s 고정
        ├── Interaction/
        │   ├── FHInteractable              # 상호작용 인터페이스 (발전기, 이후 치료/구출/출구...)
        │   ├── FHInteractionComponent      # 대상 탐색, 서버 RPC, 범위 검증
        │   └── FHSkillCheckComponent       # 스킬 체크 판정 (서버) + 표시 (클라)
        ├── Objectives/
        │   └── FHGenerator                 # 수리 진행도, 협동 효율, 스킬 체크 결과 반영
        └── UI/
            └── FHHUD                       # Canvas 디버그 HUD (에셋 없이 테스트 가능)
```

**에셋 없이 바로 돌아가게** 만들어 두었다.
- 입력: BP에 Input Action/Mapping Context를 지정하지 않으면 C++가 런타임에 기본 키를 만든다.
- 외형: 엔진 기본 도형으로 임시 몸체를 만든다 (생존자 파란 원기둥, 살인마 빨간 원기둥, 발전기 회색 박스 → 완료 시 노란색).
- UI: `AFHHUD`가 Canvas로 직접 그린다.

---

## 2. 클래스 관계

```
AFHGameMode ──(PostLogin에서 역할 결정)──► AFHPlayerState.PlayerRole
     │                                          │
     │ GetDefaultPawnClassForController ◄───────┘
     ▼
AFHSurvivorCharacter / AFHKillerCharacter  (공통 부모 AFHCharacterBase)
     ├─ UFHCharacterMovementComponent   (달리기 플래그, 이동 잠금)
     ├─ UFHInteractionComponent ──► IFHInteractable ◄── AFHGenerator
     └─ UFHSkillCheckComponent (생존자만) ──► IFHInteractable::HandleSkillCheckResult

AFHGenerator ──(완료)──► AFHGameMode::NotifyGeneratorCompleted ──► AFHGameState.GeneratorsRemaining
```

설계 원칙:
1. **판정은 전부 서버.** 클라이언트는 입력과 "몇 시에 눌렀는지"만 보낸다.
2. **상호작용은 인터페이스 하나로.** 발전기, 치료, 갈고리 구출, 출구 열기, 살인마의 발전기 차기/업기는
   모두 "누가, 무엇을, 얼마나 오래 누르고 있나"라서 `IFHInteractable`만 구현하면 된다.
3. **스킬 체크도 상호작용과 분리.** `WantsSkillChecks()`가 true인 상호작용이면 어디서든 뜬다 (2단계 치료에 재사용).

---

## 3. 핵심 흐름 (네트워크)

### 3-1. 역할 배정과 스폰
1. 접속 → `AFHGameMode::PostLogin`
2. 아직 살인마가 없으면 이 플레이어가 **살인마**, 있으면 **생존자** (PIE에서는 서버 = 살인마)
3. `GetDefaultPawnClassForController`가 역할에 맞는 캐릭터 클래스를 반환
4. `ChoosePlayerStart`가 태그 `Killer` / `Survivor`인 Player Start 중 하나를 고른다

### 3-2. 이동
| | 생존자 | 살인마 |
|---|---|---|
| 시점 | 3인칭 (SpringArm) | 1인칭 (캡슐에 카메라) |
| 회전 | 이동 방향으로 몸이 돔 | 카메라 방향 = 몸 방향 |
| 걷기 | 226 cm/s | 460 cm/s (115%) |
| 달리기 (Shift) | 400 cm/s | 없음 |
| 앉기 (Ctrl) | 113 cm/s | 없음 |

달리기는 `FSavedMove`의 `FLAG_Custom_0`으로 서버에 전달된다. 단순히 RPC로 속도만 바꾸면
서버와 클라이언트 속도가 어긋나 **러버밴딩**이 생기므로, CMC 예측 시스템에 태우는 게 정석이다.

### 3-3. 발전기 수리
```
[클라] LMB 누름
   └─ InteractionComponent::StartInteract   (bPendingInteract = true → 즉시 이동 잠금)
        └─ Server_BeginInteract(Generator)
[서버] 거리 + CanInteract 검증
   ├─ 실패 → Client_InteractRejected
   └─ 성공 → CurrentInteractable = Generator (복제), Generator::BeginInteract → Repairers에 추가
[서버] 매 Tick: Progress += 인원수 × 협동효율 × dt / 90초
         범위 이탈 / 완료 시 상호작용 강제 종료
[클라] LMB 뗌 → StopInteract (즉시 이동 해제) → Server_EndInteract
[서버] Progress ≥ 1 → 완료: 수리자 전원 해제, 노란색, GameMode에 통보 → 남은 발전기 -1
```
협동 효율(1인당): 1명 100%, 2명 85%, 3명 70%, 4명 55%. 대략값이니 플레이해 보며 조정한다.

### 3-4. 스킬 체크
```
[서버] 수리 중이면 1초마다 10% 확률로 굴림 (직전 체크 후 2초간 쿨다운)
   └─ 성공 구간 위치를 랜덤으로 정해 Client_StartSkillCheck(Params)
[클라] 0.5초 경고 → 바늘이 1.1초 동안 0→1로 이동, HUD에 표시
       Space → 현재 바늘 위치를 Server_SubmitSkillCheck(Id, Needle)로 전송
[서버] 바늘 위치 검증: 서버 기준 위치보다 늦게 누른 척하거나,
       허용 지연(0.3초)보다 더 이르게 눌렀다고 주장하면 값을 잘라냄 → 판정
       Great: +1%   Good: 변화 없음   Fail/미입력: -10%, 폭발 소리, 수리에서 튕겨남
```
Great 구간은 성공 구간 맨 앞 3%다 (원작과 같은 배치).

---

## 4. 에디터에서 할 일

### 4-1. 최초 실행
1. `FogHunt.uproject` 우클릭 → **Generate Visual Studio project files** (Mac: Generate Xcode Project)
2. IDE에서 `FogHuntEditor` / Development Editor로 빌드 → 에디터 실행
3. **File → New Level → Basic** 으로 테스트 맵을 만들고 `Content/FogHunt/Maps/L_Test`로 저장
4. 바닥을 넓히고(예: 스케일 50×50), 벽/박스를 대충 배치한다

### 4-2. 맵 배치
- **Player Start** 5개: 1개는 Details의 `Player Start Tag`를 `Killer`, 4개는 `Survivor`로 설정
- **FHGenerator** 7개: Place Actors 패널에서 `FHGenerator` 검색 후 배치 (5개를 고치면 출구 전원)
- 기존 Player Start가 하나 있다면 태그를 붙이거나 지운다

### 4-3. 멀티플레이 테스트
1. Play 버튼 옆 ⋮ → **Number of Players: 3**, **Net Mode: Play As Listen Server**
2. Play → 창 1 = 살인마(빨강, 1인칭), 창 2·3 = 생존자(파랑, 3인칭)
3. 확인할 것:
   - [ ] 생존자 걷기 / Shift 달리기 / Ctrl 앉기 속도가 다르고, 다른 창에서도 끊김 없이 보이는지
   - [ ] 살인마가 생존자 달리기보다 빠른지
   - [ ] 발전기 근처에서 `[Hold LMB] Repair` 프롬프트 → 누르고 있으면 진행 바가 차는지
   - [ ] 생존자 2명이 함께 고치면 더 빨리 차는지
   - [ ] 스킬 체크가 뜨고 Great/Good/Fail이 반영되는지, Fail 시 튕겨나는지
   - [ ] 완료 시 발전기가 노랗게 바뀌고 좌상단 남은 발전기 수가 줄어드는지
   - [ ] 살인마는 발전기 프롬프트가 안 뜨는지
4. 지연 테스트: Editor Preferences → Level Editor → Play → **Enable Network Emulation** (Average 정도)로
   스킬 체크와 이동이 지연 상황에서도 정상인지 확인

### 4-4. 에셋으로 교체할 때 (선택)
- `Content/FogHunt/Characters/`에 `BP_Survivor`(부모 `FHSurvivorCharacter`), `BP_Killer`(부모 `FHKillerCharacter`) 생성
- Skeletal Mesh와 Anim BP를 지정하고 `PlaceholderBody`의 **Hidden In Game**을 체크
- `Content/FogHunt/Input/`에 `IMC_Survivor`, `IA_Move` 등을 만들고 BP의 `FogHunt|Input` 칸에 지정
  (Mapping Context를 지정하면 런타임 기본 키는 만들어지지 않는다)
- `BP_GameMode`(부모 `FHGameMode`)를 만들어 `SurvivorClass` / `KillerClass`에 BP를 넣고,
  Project Settings → Maps & Modes의 Default GameMode를 `BP_GameMode`로 변경
- 발전기는 `BP_Generator`(부모 `FHGenerator`)에서 메시를 바꾸고 `BP_OnCompleted` / `BP_OnLoudNoise`에 사운드와 VFX 연결

권장 Content 구조:
```
Content/FogHunt/
├── Characters/{Survivor,Killer}/
├── Input/
├── Maps/
├── Objectives/Generator/
├── UI/
└── Audio/
```

---

## 5. 조정값 한눈에 보기

| 위치 | 값 | 기본 |
|---|---|---|
| FHSurvivorCharacter (CMC) | MaxWalkSpeed / MaxSprintSpeed / MaxWalkSpeedCrouched | 226 / 400 / 113 |
| FHKillerCharacter (CMC) | MaxWalkSpeed | 460 |
| FHInteractionComponent | InteractionRadius / ServerRangeTolerance | 150 / 75 |
| FHGenerator | RepairTimeSolo / CoopEfficiency / MaxRepairers | 90초 / 1, .85, .7, .55 / 4 |
| FHGenerator | GreatSkillCheckBonus / FailedSkillCheckPenalty | 0.01 / 0.10 |
| FHSkillCheckComponent | RollInterval / TriggerChance / MinTimeBetweenChecks | 1초 / 0.1 / 2초 |
| FHSkillCheckComponent | Template (Warning / Duration / GoodSize / GreatSize) | 0.5 / 1.1 / 0.13 / 0.03 |
| FHSkillCheckComponent | MaxLatencyCompensation | 0.3초 |
| FHGameMode | GeneratorsToPower | 5 |

---

## 6. 1단계에서 일부러 뺀 것 (2단계 이후)

- **살인마 공격 / 체력 상태**: `AFHCharacterBase`에 `EFHHealthState` 추가, 살인마 `IA_Attack`(LMB) + 돌진.
  발전기 차기는 `IFHInteractable`을 구현한 별도 컴포넌트, 또는 Generator의 `CanInteract`에서 역할별로 분기
- **발전기 퇴행**: 차인 뒤 수리자가 없으면 Progress를 서서히 감소
- **수리 위치 스냅**: 발전기 둘레에 수리 슬롯 4개를 두고 그 자리로 캐릭터를 이동/회전
- **실패 알림**: `Multicast_LoudNoise` 대신 살인마에게만 위치를 보내는 알림 (`Client` RPC)
- **판자 / 창틀**, 갈고리, 출구, 해치, 공포 범위(심장박동)
- 이동 잠금을 `FLAG_Custom_1`로 saved move에 넣기 (지연이 큰 환경에서의 미세 보정 제거)
- 로비 / 매치메이킹 / 데디케이티드 서버 빌드
