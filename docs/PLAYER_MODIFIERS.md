# Movimento e cadência

Esta página descreve apenas opções que executam código no jogo. Valores de interface não são tratados como funcionalidade até a cadeia de dados e a restauração estarem verificadas.

| Opção | Campo do jogo | Regra |
|---|---|---|
| Speed Hack | `PlayerMovement.walkSpeed` | Multiplica velocidade de caminhada, corrida e deslocamento horizontal no ar. Não altera movimento vertical. |
| Super Jump | `jumpSpeed` e `fallDamageThreshold` | Multiplica o impulso vertical e torna o limite negativo de queda proporcionalmente maior em magnitude. |
| Roll Speed | `rollSpeed` | Multiplica somente o deslocamento da esquiva. |
| Rapid Fire | `DatabaseGun.rof`, `fullAuto`, `burstCount` | Multiplica cadência, permite segurar armas semiautomáticas e remove a limitação de rajada. Mantém o cooldown do jogo. |

Os valores vivos confirmados nesta build são `walkSpeed=3.5`, `jumpSpeed=6`, `fallDamageThreshold=-10` e `rollSpeed=9`. `LimitVerticalVelocity` usa `-FallDamageThreshold` como teto de velocidade vertical: por isso Super Jump precisa preservar o sinal negativo.

Cada campo modificado registra seu valor inicial por objeto e volta ao original ao desligar a opção ou trocar de cena. Rapid Fire também restaura `fullAuto` e `burstCount`; não força mais `PhysicalGun.Cooldown` para zero a cada ciclo.

Limites do menu existem para evitar valores inviáveis: Speed e Roll 1x–5x; Jump 1x–10x; Rapid 1x–5x. Comece com 1.5x–2x. O estado real é registrado no log pelos marcadores `[MOVE]`, `[JUMP]`, `[WEAPON]` e `[RESTORE]`.

Evidência: decompilação de `PlayerMovement.SetBodyVelocity`, `JumpState`, `AirState` e `LimitVerticalVelocity`; offsets vivos conferidos pelo Mono do jogo em 25/09. Nenhum offset numérico novo é usado para escrita: o código resolve campos por nome na API Mono.
