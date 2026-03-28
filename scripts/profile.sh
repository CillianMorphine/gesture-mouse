#!/usr/bin/env bash
# scripts/profile.sh
# ─────────────────────────────────────────────────────────────────────────────
# Скрипт для запуску зовнішніх інструментів профілювання GestureMouse на Linux.
# Вимагає: valgrind, gprof, graphviz, gprof2dot (pip install gprof2dot)
# ─────────────────────────────────────────────────────────────────────────────

set -e
BUILD_DIR="build"
OUT_DIR="profiling_results"
mkdir -p "$OUT_DIR"

echo "=== GestureMouse Profiling Script ==="
echo ""

# ── 1. Callgrind (детальний CPU-профіль) ─────────────────────────────────────
echo "[1/3] Callgrind (CPU profiling)..."
if command -v valgrind &>/dev/null; then
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo \
          -DCMAKE_CXX_FLAGS="-fno-omit-frame-pointer" \
          -B "$BUILD_DIR" -S . -DGM_PROFILING_ENABLED=ON
    cmake --build "$BUILD_DIR" -j4

    valgrind \
        --tool=callgrind \
        --callgrind-out-file="$OUT_DIR/callgrind.out" \
        --collect-atstart=yes \
        --instr-atstart=yes \
        "$BUILD_DIR/gesture_mouse" --log-level=warn &
    VPID=$!
    sleep 10   # дати програмі попрацювати
    kill $VPID 2>/dev/null || true

    echo "  Callgrind результат: $OUT_DIR/callgrind.out"
    echo "  Перегляд: kcachegrind $OUT_DIR/callgrind.out"
else
    echo "  [SKIP] valgrind не знайдено. Встановіть: sudo apt install valgrind"
fi

# ── 2. gprof (call graph) ────────────────────────────────────────────────────
echo ""
echo "[2/3] gprof (call graph)..."
if command -v gprof &>/dev/null; then
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo \
          -DCMAKE_CXX_FLAGS="-pg -fno-omit-frame-pointer" \
          -B "${BUILD_DIR}_gprof" -S .
    cmake --build "${BUILD_DIR}_gprof" -j4

    "${BUILD_DIR}_gprof/gesture_mouse" --log-level=warn &
    GPID=$!
    sleep 10
    kill $GPID 2>/dev/null || true

    gprof "${BUILD_DIR}_gprof/gesture_mouse" gmon.out \
        > "$OUT_DIR/gprof_report.txt"

    if command -v gprof2dot &>/dev/null && command -v dot &>/dev/null; then
        gprof "${BUILD_DIR}_gprof/gesture_mouse" gmon.out \
            | gprof2dot \
            | dot -Tpng -o "$OUT_DIR/callgraph.png"
        echo "  Call graph: $OUT_DIR/callgraph.png"
    fi
    echo "  gprof звіт: $OUT_DIR/gprof_report.txt"
else
    echo "  [SKIP] gprof не знайдено (входить до складу gcc)"
fi

# ── 3. AddressSanitizer (витоки пам'яті) ─────────────────────────────────────
echo ""
echo "[3/3] AddressSanitizer (memory leaks)..."
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=address,leak -fno-omit-frame-pointer -g" \
      -B "${BUILD_DIR}_asan" -S .
cmake --build "${BUILD_DIR}_asan" -j4

ASAN_OPTIONS=log_path="$OUT_DIR/asan" \
    "${BUILD_DIR}_asan/gesture_mouse" --log-level=warn &
APID=$!
sleep 5
kill $APID 2>/dev/null || true
echo "  ASan звіт: $OUT_DIR/asan.*"

# ── 4. Мікробенчмарк (вбудований) ────────────────────────────────────────────
echo ""
echo "[4] Запуск мікробенчмарків..."
cmake -DCMAKE_BUILD_TYPE=Release -B "${BUILD_DIR}_bench" -S .
cmake --build "${BUILD_DIR}_bench" --target gesture_mouse_bench -j4
cmake --build "${BUILD_DIR}_bench" --target gesture_mouse_bench_compare -j4

"${BUILD_DIR}_bench/gesture_mouse_bench"         | tee "$OUT_DIR/bench_baseline.txt"
"${BUILD_DIR}_bench/gesture_mouse_bench_compare" | tee "$OUT_DIR/bench_comparison.txt"

echo ""
echo "=== Профілювання завершено. Результати: $OUT_DIR/ ==="
