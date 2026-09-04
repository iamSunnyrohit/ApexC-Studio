package com.example.apexc

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.BackHandler
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

class MainActivity : ComponentActivity() {

    companion object {
        init {
            System.loadLibrary("apexc")
        }
    }

    private external fun evalSource(source: String): String
    private external fun compileToAsm(source: String): String

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            var selectedTab by remember { mutableStateOf("ARCH") }
            var activeMode by remember { mutableStateOf("RUN") }

            Surface(
                modifier = Modifier.fillMaxSize(),
                color = Color(0xFFF6F8FA)
            ) {
                Scaffold(
                    modifier = Modifier.fillMaxSize(),
                    containerColor = Color(0xFFF6F8FA),
                    bottomBar = {
                        ApexBottomBar(
                            currentTab = selectedTab,
                            onSelectTab = { selectedTab = it }
                        )
                    }
                ) { innerPadding ->
                    when (selectedTab) {
                        "ARCH" -> {
                            ArchitectureHomeScreen(
                                modifier = Modifier.padding(innerPadding),
                                onLaunchIDE = {
                                    activeMode = "RUN"
                                    selectedTab = "EDITOR"
                                },
                                onQuickRun = {
                                    activeMode = "RUN"
                                    selectedTab = "EDITOR"
                                },
                                onLiveDisasm = {
                                    activeMode = "ASM"
                                    selectedTab = "EDITOR"
                                }
                            )
                        }
                        "EDITOR" -> {
                            BackHandler { selectedTab = "ARCH" }
                            CompilerStudioScreen(
                                modifier = Modifier.padding(innerPadding),
                                initialMode = activeMode,
                                onRun = { evalSource(it) },
                                onDisassemble = { compileToAsm(it) }
                            )
                        }
                    }
                }
            }
        }
    }
}

// -------------------------------------------------------------
// REUSABLE BADGES & TELEMETRY
// -------------------------------------------------------------

@Composable
fun TelemetryBadge(text: String, isGreen: Boolean = true) {
    Row(
        verticalAlignment = Alignment.CenterVertically,
        modifier = Modifier
            .background(Color(0xFFF0FDF4), RoundedCornerShape(20.dp))
            .border(1.dp, Color(0xFFDCFCE7), RoundedCornerShape(20.dp))
            .padding(horizontal = 8.dp, vertical = 3.dp)
    ) {
        Box(
            modifier = Modifier
                .size(6.dp)
                .background(if (isGreen) Color(0xFF22C55E) else Color(0xFF0EA5E9), CircleShape)
        )
        Spacer(modifier = Modifier.width(5.dp))
        Text(
            text = text,
            fontFamily = FontFamily.Monospace,
            fontSize = 9.sp,
            fontWeight = FontWeight.Bold,
            color = Color(0xFF166534)
        )
    }
}

@Composable
fun PillBadge(text: String, textColor: Color, bgColor: Color) {
    Box(
        modifier = Modifier
            .background(bgColor, RoundedCornerShape(6.dp))
            .padding(horizontal = 6.dp, vertical = 2.dp)
    ) {
        Text(
            text = text,
            fontFamily = FontFamily.Monospace,
            fontSize = 9.sp,
            fontWeight = FontWeight.Bold,
            color = textColor
        )
    }
}

// -------------------------------------------------------------
// ARCHITECTURE HOME SCREEN
// -------------------------------------------------------------

@Composable
fun ArchitectureHomeScreen(
    modifier: Modifier = Modifier,
    onLaunchIDE: () -> Unit,
    onQuickRun: () -> Unit,
    onLiveDisasm: () -> Unit
) {
    LazyColumn(
        modifier = modifier
            .fillMaxSize()
            .padding(horizontal = 16.dp),
        verticalArrangement = Arrangement.spacedBy(14.dp),
        contentPadding = PaddingValues(top = 10.dp, bottom = 24.dp)
    ) {
        // Top Global Status Header
        item {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Box(
                        modifier = Modifier
                            .size(28.dp)
                            .background(Color(0xFFE0F2FE), RoundedCornerShape(6.dp)),
                        contentAlignment = Alignment.Center
                    ) {
                        Text("⚙", fontSize = 14.sp)
                    }
                    Spacer(modifier = Modifier.width(8.dp))
                    Column {
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Text(
                                "ApexC",
                                fontWeight = FontWeight.ExtraBold,
                                fontSize = 16.sp,
                                color = Color(0xFF0F172A)
                            )
                            Spacer(modifier = Modifier.width(4.dp))
                            PillBadge("C11", Color(0xFF0284C7), Color(0xFFE0F2FE))
                        }
                        Text(
                            "• AARCH64 NATIVE",
                            fontFamily = FontFamily.Monospace,
                            fontSize = 9.sp,
                            fontWeight = FontWeight.Bold,
                            color = Color(0xFF65A30D)
                        )
                    }
                }

                Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                    Box(
                        modifier = Modifier
                            .size(32.dp)
                            .background(Color(0xFFF1F5F9), RoundedCornerShape(8.dp)),
                        contentAlignment = Alignment.Center
                    ) { Text("⌨", fontSize = 14.sp) }
                    Box(
                        modifier = Modifier
                            .size(32.dp)
                            .background(Color(0xFFF1F5F9), RoundedCornerShape(8.dp)),
                        contentAlignment = Alignment.Center
                    ) { Text("⚙", fontSize = 14.sp) }
                    Box(
                        modifier = Modifier
                            .size(32.dp)
                            .background(Color(0xFF0284C7), CircleShape),
                        contentAlignment = Alignment.Center
                    ) { Text("👤", fontSize = 14.sp) }
                }
            }
        }

        // Sub Status Strip
        item {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                TelemetryBadge("● ONLINE • ARM64 / AAPCS64")
                Box(
                    modifier = Modifier
                        .background(Color(0xFFF8FAFC), RoundedCornerShape(12.dp))
                        .border(1.dp, Color(0xFFE2E8F0), RoundedCornerShape(12.dp))
                        .padding(horizontal = 8.dp, vertical = 3.dp)
                ) {
                    Text(
                        "SYS_OK 0x0",
                        fontFamily = FontFamily.Monospace,
                        fontSize = 9.sp,
                        fontWeight = FontWeight.Bold,
                        color = Color(0xFF0284C7)
                    )
                }
            }
        }

        // Hero Studio Card
        item {
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .clip(RoundedCornerShape(18.dp))
                    .background(
                        Brush.verticalGradient(
                            listOf(Color(0xFFF0FDF4).copy(alpha = 0.5f), Color.White)
                        )
                    )
                    .border(1.dp, Color(0xFFE2E8F0), RoundedCornerShape(18.dp))
                    .padding(18.dp)
            ) {
                Column {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Box(
                            modifier = Modifier
                                .size(42.dp)
                                .background(Color(0xFF0F172A), RoundedCornerShape(10.dp)),
                            contentAlignment = Alignment.Center
                        ) {
                            Text("A", color = Color(0xFF38BDF8), fontWeight = FontWeight.Black, fontSize = 22.sp)
                        }
                        Spacer(modifier = Modifier.width(12.dp))
                        Column {
                            Row(verticalAlignment = Alignment.CenterVertically) {
                                Text(
                                    "ApexC Studio",
                                    fontSize = 18.sp,
                                    fontWeight = FontWeight.Bold,
                                    color = Color(0xFF0F172A)
                                )
                                Spacer(modifier = Modifier.width(6.dp))
                                PillBadge("C11 READY", Color(0xFF0369A1), Color(0xFFE0F2FE))
                            }
                            Text(
                                "AArch64 COMPILER TOOLCHAIN",
                                fontFamily = FontFamily.Monospace,
                                fontSize = 9.sp,
                                fontWeight = FontWeight.Bold,
                                color = Color(0xFF65A30D)
                            )
                        }
                    }

                    Spacer(modifier = Modifier.height(12.dp))

                    Text(
                        "A lightweight C11 compiler targeting native ARM64 assembly with an embedded AST runtime engine.",
                        fontSize = 12.sp,
                        color = Color(0xFF475569),
                        lineHeight = 17.sp
                    )

                    Spacer(modifier = Modifier.height(14.dp))

                    // Spec Grid
                    Column(
                        modifier = Modifier
                            .fillMaxWidth()
                            .background(Color(0xFFF8FAFC), RoundedCornerShape(10.dp))
                            .border(1.dp, Color(0xFFF1F5F9), RoundedCornerShape(10.dp))
                            .padding(horizontal = 12.dp, vertical = 8.dp),
                        verticalArrangement = Arrangement.spacedBy(6.dp)
                    ) {
                        Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
                            SpecRowItem("ABI", "AAPCS64", Color(0xFF0284C7))
                            SpecRowItem("STD", "C11", Color(0xFF0F172A))
                        }
                        Divider(color = Color(0xFFE2E8F0), thickness = 0.5.dp)
                        Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
                            SpecRowItem("STACK", "16-BYTE", Color(0xFF0284C7))
                            SpecRowItem("JIT", "DUAL-MODE", Color(0xFF65A30D))
                        }
                    }
                }
            }
        }

        // Action CTAs
        item {
            Button(
                onClick = onLaunchIDE,
                modifier = Modifier
                    .fillMaxWidth()
                    .height(48.dp),
                shape = RoundedCornerShape(12.dp),
                colors = ButtonDefaults.buttonColors(containerColor = Color(0xFF0891B2))
            ) {
                Text("🚀 Launch Compiler IDE →", fontWeight = FontWeight.Bold, fontSize = 14.sp)
            }
        }

        item {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(10.dp)
            ) {
                OutlinedButton(
                    onClick = onQuickRun,
                    modifier = Modifier.weight(1f),
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.outlinedButtonColors(contentColor = Color(0xFF0F172A)),
                    border = BorderStroke(1.dp, Color(0xFFE2E8F0))
                ) {
                    Text("▷ Quick Run AST", fontSize = 11.sp, fontWeight = FontWeight.SemiBold, fontFamily = FontFamily.Monospace)
                }

                OutlinedButton(
                    onClick = onLiveDisasm,
                    modifier = Modifier.weight(1f),
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.outlinedButtonColors(contentColor = Color(0xFF0891B2)),
                    border = BorderStroke(1.dp, Color(0xFFE2E8F0))
                ) {
                    Text("⇄ Live Disasm", fontSize = 11.sp, fontWeight = FontWeight.SemiBold, fontFamily = FontFamily.Monospace)
                }
            }
        }

        // Section Title
        item {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text("🔀", fontSize = 12.sp)
                    Spacer(modifier = Modifier.width(6.dp))
                    Text(
                        "SYSTEM\nARCHITECTURE",
                        fontSize = 11.sp,
                        fontWeight = FontWeight.ExtraBold,
                        color = Color(0xFF0F172A),
                        lineHeight = 13.sp
                    )
                }
                Box(
                    modifier = Modifier
                        .background(Color(0xFFF1F5F9), RoundedCornerShape(6.dp))
                        .padding(horizontal = 8.dp, vertical = 4.dp)
                ) {
                    Text(
                        "SPEC v2.4 • TARGET: CORTEX-A",
                        fontFamily = FontFamily.Monospace,
                        fontSize = 8.sp,
                        fontWeight = FontWeight.Bold,
                        color = Color(0xFF475569)
                    )
                }
            }
        }

        // Architecture Cards
        item {
            ArchitectureCard(
                category = "HARDWARE ABI",
                badge = "CORE SPEC",
                badgeBg = Color(0xFFECFCCB),
                badgeColor = Color(0xFF4D7C0F),
                title = "AAPCS64 Stack Conformance",
                description = "Strict 16-byte stack boundary enforcement, x29/x30 preservation across arbitrary call depth.",
                snippet = "stp x29, x30, [sp, #-16]!"
            )
        }

        item {
            ArchitectureCard(
                category = "EXECUTION ENGINE",
                badge = "RUNTIME",
                badgeBg = Color(0xFFE0F2FE),
                badgeColor = Color(0xFF0284C7),
                title = "Dual Mode (Runtime Eval & Disassembly)",
                description = "Direct in-memory AST evaluator for return values alongside raw AArch64 machine code generation.",
                toggleLeft = "JIT Eval",
                toggleRight = "Raw Asm"
            )
        }

        item {
            ArchitectureCard(
                category = "FRONTEND PIPELINE",
                badge = "PARSER",
                badgeBg = Color(0xFFE0F2FE),
                badgeColor = Color(0xFF0284C7),
                title = "Recursive Descent Parser",
                description = "Precedence-climbing AST generator without any Lex/Yacc/Bison external toolchain dependencies.",
                pills = listOf("● Zero Dependencies", "Standalone")
            )
        }

        item {
            ArchitectureCard(
                category = "C11 SUBSET",
                badge = "LANGUAGE",
                badgeBg = Color(0xFFE0F2FE),
                badgeColor = Color(0xFF0284C7),
                title = "Structured Control Flow",
                description = "Nested calls, conditionals (cmp, cset, b.eq), and local stack-frame variable resolution.",
                pills = listOf("cmp", "cset", "b.eq", "ldr/str")
            )
        }

        // Bottom Telemetry Footer
        item {
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .background(Color.White, RoundedCornerShape(10.dp))
                    .border(1.dp, Color(0xFFE2E8F0), RoundedCornerShape(10.dp))
                    .padding(vertical = 8.dp, horizontal = 12.dp)
            ) {
                Text(
                    "✓ target: cortex-a78 • opt: -O2 • lld-re... 0.04ms",
                    fontFamily = FontFamily.Monospace,
                    fontSize = 9.sp,
                    color = Color(0xFF16A34A),
                    fontWeight = FontWeight.SemiBold
                )
            }
        }
    }
}

@Composable
fun SpecRowItem(label: String, value: String, valueColor: Color) {
    Row(
        modifier = Modifier.width(140.dp),
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Text(label, fontFamily = FontFamily.Monospace, fontSize = 9.sp, color = Color(0xFF94A3B8), fontWeight = FontWeight.Bold)
        Text(value, fontFamily = FontFamily.Monospace, fontSize = 9.sp, color = valueColor, fontWeight = FontWeight.Bold)
    }
}

@Composable
fun ArchitectureCard(
    category: String,
    badge: String,
    badgeBg: Color,
    badgeColor: Color,
    title: String,
    description: String,
    snippet: String? = null,
    toggleLeft: String? = null,
    toggleRight: String? = null,
    pills: List<String>? = null
) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        shape = RoundedCornerShape(16.dp),
        colors = CardDefaults.cardColors(containerColor = Color.White),
        border = BorderStroke(1.dp, Color(0xFFE2E8F0))
    ) {
        Column(modifier = Modifier.padding(14.dp)) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text("❖", fontSize = 10.sp, color = Color(0xFF0284C7))
                    Spacer(modifier = Modifier.width(4.dp))
                    Text(category, fontFamily = FontFamily.Monospace, fontSize = 9.sp, fontWeight = FontWeight.Bold, color = Color(0xFF0284C7))
                }
                PillBadge(badge, badgeColor, badgeBg)
            }

            Spacer(modifier = Modifier.height(6.dp))
            Text(title, fontWeight = FontWeight.Bold, fontSize = 14.sp, color = Color(0xFF0F172A))
            Spacer(modifier = Modifier.height(4.dp))
            Text(description, fontSize = 11.sp, color = Color(0xFF64748B), lineHeight = 16.sp)

            snippet?.let {
                Spacer(modifier = Modifier.height(10.dp))
                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .background(Color(0xFFF8FAFC), RoundedCornerShape(8.dp))
                        .border(1.dp, Color(0xFFE2E8F0), RoundedCornerShape(8.dp))
                        .padding(horizontal = 10.dp, vertical = 6.dp)
                ) {
                    Text(it, fontFamily = FontFamily.Monospace, fontSize = 10.sp, color = Color(0xFF0F172A))
                }
            }

            if (toggleLeft != null && toggleRight != null) {
                Spacer(modifier = Modifier.height(10.dp))
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .background(Color(0xFFF1F5F9), RoundedCornerShape(8.dp))
                        .padding(2.dp)
                ) {
                    Box(
                        modifier = Modifier
                            .weight(1f)
                            .background(Color.White, RoundedCornerShape(6.dp))
                            .padding(vertical = 4.dp),
                        contentAlignment = Alignment.Center
                    ) {
                        Text(toggleLeft, fontFamily = FontFamily.Monospace, fontSize = 10.sp, fontWeight = FontWeight.Bold, color = Color(0xFF0F172A))
                    }
                    Box(
                        modifier = Modifier
                            .weight(1f)
                            .padding(vertical = 4.dp),
                        contentAlignment = Alignment.Center
                    ) {
                        Text(toggleRight, fontFamily = FontFamily.Monospace, fontSize = 10.sp, color = Color(0xFF64748B))
                    }
                }
            }

            pills?.let {
                Spacer(modifier = Modifier.height(10.dp))
                Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                    it.forEach { pill ->
                        Box(
                            modifier = Modifier
                                .background(Color(0xFFF8FAFC), RoundedCornerShape(6.dp))
                                .border(1.dp, Color(0xFFE2E8F0), RoundedCornerShape(6.dp))
                                .padding(horizontal = 8.dp, vertical = 3.dp)
                        ) {
                            Text(pill, fontFamily = FontFamily.Monospace, fontSize = 9.sp, fontWeight = FontWeight.SemiBold, color = Color(0xFF0F172A))
                        }
                    }
                }
            }
        }
    }
}

// -------------------------------------------------------------
// COMPILER STUDIO SCREEN
// -------------------------------------------------------------

data class CodePreset(val label: String, val code: String)

@Composable
fun CompilerStudioScreen(
    modifier: Modifier = Modifier,
    initialMode: String = "RUN",
    onRun: (String) -> String,
    onDisassemble: (String) -> String
) {
    val presets = remember {
        listOf(
            CodePreset(
                "Hello World",
                "#include <stdio.h>\n\nint main() {\n    printf(\"Hello, World!\\n\");\n    return 0;\n}"
            ),
            CodePreset(
                "Math Lib",
                "#include <stdio.h>\n#include <math.h>\n\nint main() {\n    int val = 49;\n    int r = sqrt(val);\n    printf(\"sqrt(%d) = %d\\n\", val, r);\n    return r;\n}"
            ),
            CodePreset(
                "Fibonacci",
                "int fib(int n) {\n    if (n <= 1) return n;\n    return fib(n - 1) + fib(n - 2);\n}\n\nint main() {\n    return fib(7);\n}"
            ),
            CodePreset(
                "Loop Accumulator",
                "int main() {\n    int sum = 0;\n    int i = 1;\n    while (i <= 10) {\n        sum = sum + i;\n        i = i + 1;\n    }\n    return sum;\n}"
            )
        )
    }

    var sourceCode by remember { mutableStateOf(presets[0].code) }
    var outputText by remember { mutableStateOf("") }
    var isAsmMode by remember { mutableStateOf(initialMode == "ASM") }

    fun execute(asm: Boolean, src: String) {
        isAsmMode = asm
        outputText = if (asm) onDisassemble(src) else onRun(src)
    }

    LaunchedEffect(initialMode) {
        execute(initialMode == "ASM", sourceCode)
    }

    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(16.dp)
    ) {
        // Preset Chips
        LazyRow(
            horizontalArrangement = Arrangement.spacedBy(8.dp),
            modifier = Modifier.fillMaxWidth()
        ) {
            items(presets) { preset ->
                FilterChip(
                    selected = sourceCode == preset.code,
                    onClick = {
                        sourceCode = preset.code
                        execute(isAsmMode, preset.code)
                    },
                    label = { Text(preset.label, fontSize = 11.sp, fontFamily = FontFamily.Monospace) },
                    colors = FilterChipDefaults.filterChipColors(
                        selectedContainerColor = Color(0xFFE0F2FE),
                        selectedLabelColor = Color(0xFF0369A1),
                        containerColor = Color.White,
                        labelColor = Color(0xFF64748B)
                    )
                )
            }
        }

        Spacer(modifier = Modifier.height(10.dp))

        // Monospace Source Editor
        Box(
            modifier = Modifier
                .weight(1.2f)
                .fillMaxWidth()
                .background(Color.White, RoundedCornerShape(12.dp))
                .border(1.dp, Color(0xFFE2E8F0), RoundedCornerShape(12.dp))
                .padding(12.dp)
        ) {
            val lines = sourceCode.lines()
            Row(modifier = Modifier.fillMaxSize()) {
                Column(
                    modifier = Modifier
                        .verticalScroll(rememberScrollState())
                        .padding(end = 12.dp)
                ) {
                    for (i in 1..lines.size) {
                        Text(
                            text = "$i",
                            color = Color(0xFF94A3B8),
                            fontFamily = FontFamily.Monospace,
                            fontSize = 12.sp,
                            lineHeight = 18.sp
                        )
                    }
                }

                BasicTextField(
                    value = sourceCode,
                    onValueChange = { sourceCode = it },
                    modifier = Modifier
                        .fillMaxSize()
                        .verticalScroll(rememberScrollState())
                        .horizontalScroll(rememberScrollState()),
                    textStyle = TextStyle(
                        color = Color(0xFF0F172A),
                        fontFamily = FontFamily.Monospace,
                        fontSize = 12.sp,
                        lineHeight = 18.sp
                    ),
                    cursorBrush = SolidColor(Color(0xFF0284C7))
                )
            }
        }

        Spacer(modifier = Modifier.height(10.dp))

        // Dual Action Controls
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(10.dp)
        ) {
            Button(
                onClick = { execute(false, sourceCode) },
                modifier = Modifier.weight(1f),
                shape = RoundedCornerShape(10.dp),
                colors = ButtonDefaults.buttonColors(
                    containerColor = if (!isAsmMode) Color(0xFF0891B2) else Color(0xFFF1F5F9),
                    contentColor = if (!isAsmMode) Color.White else Color(0xFF475569)
                )
            ) {
                Text("▷ Run AST", fontWeight = FontWeight.Bold, fontFamily = FontFamily.Monospace)
            }

            Button(
                onClick = { execute(true, sourceCode) },
                modifier = Modifier.weight(1f),
                shape = RoundedCornerShape(10.dp),
                colors = ButtonDefaults.buttonColors(
                    containerColor = if (isAsmMode) Color(0xFF0891B2) else Color(0xFFF1F5F9),
                    contentColor = if (isAsmMode) Color.White else Color(0xFF475569)
                )
            ) {
                Text("⇄ Disassemble", fontWeight = FontWeight.Bold, fontFamily = FontFamily.Monospace)
            }
        }

        Spacer(modifier = Modifier.height(10.dp))

        // Output Display Box
        Column(
            modifier = Modifier
                .weight(0.9f)
                .fillMaxWidth()
                .background(Color(0xFF0F172A), RoundedCornerShape(12.dp))
                .padding(12.dp)
        ) {
            Text(
                text = if (isAsmMode) "DISASSEMBLY OUTPUT (.s)" else "EVALUATION OUTPUT",
                fontFamily = FontFamily.Monospace,
                fontSize = 10.sp,
                fontWeight = FontWeight.Bold,
                color = Color(0xFF94A3B8)
            )
            Spacer(modifier = Modifier.height(6.dp))
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .verticalScroll(rememberScrollState())
                    .horizontalScroll(rememberScrollState())
            ) {
                Text(
                    text = outputText,
                    fontFamily = FontFamily.Monospace,
                    fontSize = 12.sp,
                    color = if (isAsmMode) Color(0xFF38BDF8) else Color(0xFF4ADE80),
                    lineHeight = 18.sp
                )
            }
        }
    }
}

// -------------------------------------------------------------
// BOTTOM NAVIGATION BAR (2 Tabs: HOME & EDITOR)
// -------------------------------------------------------------

@Composable
fun ApexBottomBar(
    currentTab: String,
    onSelectTab: (String) -> Unit
) {
    val items = listOf(
        Pair("ARCH", "⊞"),
        Pair("EDITOR", "< >")
    )

    Surface(
        modifier = Modifier.fillMaxWidth(),
        color = Color.White,
        border = BorderStroke(0.5.dp, Color(0xFFE2E8F0))
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .navigationBarsPadding()
                .padding(vertical = 10.dp),
            horizontalArrangement = Arrangement.SpaceEvenly,
            verticalAlignment = Alignment.CenterVertically
        ) {
            items.forEach { (key, icon) ->
                val isSelected = currentTab == key
                Column(
                    horizontalAlignment = Alignment.CenterHorizontally,
                    modifier = Modifier
                        .weight(1f)
                        .clickable(
                            interactionSource = null,
                            indication = null
                        ) { onSelectTab(key) }
                ) {
                    Text(
                        text = icon,
                        fontSize = 16.sp,
                        fontWeight = FontWeight.Bold,
                        color = if (isSelected) Color(0xFF0891B2) else Color(0xFF94A3B8)
                    )
                    Spacer(modifier = Modifier.height(3.dp))
                    Text(
                        text = if (key == "ARCH") "HOME" else "EDITOR",
                        fontFamily = FontFamily.Monospace,
                        fontSize = 10.sp,
                        fontWeight = if (isSelected) FontWeight.Bold else FontWeight.Medium,
                        color = if (isSelected) Color(0xFF0891B2) else Color(0xFF94A3B8)
                    )
                }
            }
        }
    }
}