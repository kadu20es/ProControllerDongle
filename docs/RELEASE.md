# Como gerar e publicar uma versão

## 1. Gerar os arquivos

```powershell
powershell -File tools\package-release.ps1
```

O script compila o firmware **release** e o **debug** e cria a pasta `dist\` (ignorada pelo git) com:

| Arquivo | Conteúdo |
|---|---|
| `ProControllerDongle-vX.Y.Z.uf2` | Firmware release (uso diário). **É o arquivo principal** |
| `ProControllerDongle-vX.Y.Z-debug.uf2` | Firmware debug (porta serial de log) |
| `SHA256SUMS.txt` | Somas de verificação |
| `THIRD-PARTY-NOTICES.md` | Licenças de terceiros. **Distribua sempre junto do `.uf2`** |
| `RELEASE_NOTES-vX.Y.Z.md` | Texto pronto para a página do release |

A versão vem de `PROCON_VERSION` em `firmware/CMakeLists.txt`.

## 2. Publicar no GitHub

```sh
git remote add origin https://github.com/<usuario>/<repositorio>.git
git push -u origin main --tags
```

Depois, em **Releases > Draft a new release**, escolha a tag `vX.Y.Z`, cole o `RELEASE_NOTES-vX.Y.Z.md` e anexe os
arquivos `.uf2`, o `SHA256SUMS.txt` e o `THIRD-PARTY-NOTICES.md`. Ou, com o GitHub CLI:

```sh
gh release create vX.Y.Z dist/ProControllerDongle-vX.Y.Z.uf2 dist/ProControllerDongle-vX.Y.Z-debug.uf2 dist/SHA256SUMS.txt dist/THIRD-PARTY-NOTICES.md \
   --title "Pro Controller Dongle vX.Y.Z" --notes-file dist/RELEASE_NOTES-vX.Y.Z.md
```

## 3. Nova versão
1. Altere `PROCON_VERSION` em `firmware/CMakeLists.txt`.
2. Atualize o `CHANGELOG.md`.
3. Faça o commit, crie a tag (`git tag -a vX.Y.Z -m "..."`), rode o `package-release.ps1` e publique.

## Antes de publicar
- Teste o `.uf2` release em uma Pico 2 W limpa (BOOTSEL > copiar) e confirme o pareamento, a reconexão e o wake.
- Confira se nenhum arquivo local (`tools\env.cmd`, `dist\`, pastas `build*`) entrou no commit (`git status`).
