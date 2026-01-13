# ChatGPT KRunner Plugin for KDE Plasma 6

<p align="center">
  <img src="https://img.shields.io/badge/KDE-Plasma%206-blue?logo=kde" alt="KDE Plasma 6"/>
  <img src="https://img.shields.io/badge/Qt-6-green?logo=qt" alt="Qt 6"/>
  <img src="https://img.shields.io/badge/License-MIT-yellow" alt="MIT License"/>
</p>

A KRunner plugin that lets you query ChatGPT (or any OpenAI-compatible LLM) directly from KRunner in KDE Plasma 6.

## ✨ Features

- 🚀 **Quick Access**: Type `gpt <your question>` in KRunner (Alt+Space)
- ⚡ **Async Queries**: Non-blocking API calls with "Thinking..." placeholder
- 💾 **Smart Caching**: Remembers last 20 queries for instant results
- ⏱️ **Debouncing**: Waits 350ms after you stop typing to avoid spamming API
- 📋 **Actions**:
  - **Default (Enter)**: Copy answer to clipboard
  - **Open in Browser**: Launch ChatGPT web interface
- 🔧 **Flexible Backend**: Works with OpenAI, local LLMs, or any OpenAI-compatible API

## 📋 Environment Variables

Configure the plugin using these environment variables:

| Variable | Description | Default |
|----------|-------------|---------||
| `GPT_KRUNNER_API_KEY` | **Required**. Your OpenAI API key or compatible API key | (none) |
| `GPT_KRUNNER_BASE_URL` | API base URL | `https://api.openai.com/v1` |
| `GPT_KRUNNER_MODEL` | Model name | `gpt-4o-mini` |

### Example Configuration

Add to your `~/.bashrc`, `~/.zshrc`, or `~/.config/plasma-workspace/env/gpt-runner.sh`:

```bash
export GPT_KRUNNER_API_KEY="sk-your-api-key-here"
export GPT_KRUNNER_MODEL="gpt-4o-mini"
# For local LLM (e.g., LM Studio, Ollama with OpenAI compat):
# export GPT_KRUNNER_BASE_URL="http://localhost:1234/v1"
```

**Important**: Restart your Plasma session after changing environment variables.

## 🔨 Build Instructions (Fedora/KDE Plasma 6)

### 1. Install Dependencies

```bash
sudo dnf install \
    cmake \
    extra-cmake-modules \
    kf6-krunner-devel \
    kf6-ki18n-devel \
    kf6-kcoreaddons-devel \
    kf6-kconfig-devel \
    qt6-qtbase-devel \
    qt6-qtnetwork-devel \
    gcc-c++
```

### 2. Build

```bash
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
make
```

### 3. Install

```bash
sudo make install
```

The plugin will be installed to:
```
/usr/lib64/qt6/plugins/kf6/krunner/krunner_chatgpt.so
/usr/lib64/qt6/plugins/kf6/krunner/chatgptrunner.json
```

(Path may vary: `/usr/lib64/` on Fedora, `/usr/lib/` on some distros)

### 4. Restart KRunner

```bash
kquitapp6 krunner
kstart6 krunner
```

Or log out and back in to restart your Plasma session.

## 🚀 Usage

1. Press **Alt+Space** (or Meta) to open KRunner
2. Type: `gpt what is the speed of light?`
3. Wait ~1 second for the answer to appear
4. Press **Enter** to copy the answer to clipboard
5. Or use **Tab** to select "Open in Browser" action

### Examples

```
gpt what is 2+2
gpt explain quantum entanglement in simple terms
gpt write a haiku about coffee
```

## 🔧 Troubleshooting

### "Missing GPT_KRUNNER_API_KEY" error

- Set the environment variable in a shell startup file
- Restart your Plasma session (not just KRunner)
- Verify: `echo $GPT_KRUNNER_API_KEY` in Konsole

### No results appear

```bash
# Check if plugin is installed
ls /usr/lib64/qt6/plugins/kf6/krunner/krunner_chatgpt.so

# Check KRunner logs
journalctl --user -f | grep -i krunner

# Reinstall
sudo make install
kquitapp6 krunner && kstart6 krunner
```

### Using Local LLM (LM Studio, Ollama, etc.)

```bash
# Example for LM Studio (OpenAI-compatible endpoint)
export GPT_KRUNNER_BASE_URL="http://localhost:1234/v1"
export GPT_KRUNNER_API_KEY="lm-studio"  # Any non-empty value
export GPT_KRUNNER_MODEL="local-model"  # Your loaded model name
```

## 🗑️ Uninstall

```bash
sudo rm /usr/lib64/qt6/plugins/kf6/krunner/krunner_chatgpt.so
sudo rm /usr/lib64/qt6/plugins/kf6/krunner/chatgptrunner.json
kquitapp6 krunner && kstart6 krunner
```

## 📄 License

MIT License - Feel free to modify and distribute.

## 🤝 Contributing

Contributions welcome! Possible improvements:

- Streaming responses
- Conversation history
- GUI config dialog
- Custom system prompts
- Multiple models

## 📧 Support

For issues and feature requests, please use the [GitHub Issues](https://github.com/zany130/chatgpt-krunner/issues) page.