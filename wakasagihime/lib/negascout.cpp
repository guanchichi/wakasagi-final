#include "negascout.h"
#include "star.h"
#include "chess.h"
#include "types.h"
#include "helper.h"
#include "zobrist.h"
#include "tt.h"
#include "evaluate.h"
#include <vector>
#include <algorithm>
#include <limits>
#include <chrono>

// Extern declarations for global time management variables
extern std::chrono::time_point<std::chrono::steady_clock> stop_time;
extern bool time_up;
extern long long nodes_visited;


float F4_NegaScout(Position& pos, uint64_t key, int depth, float alpha, float beta, Color mySide) {
    // 每 2048 個節點檢查一次時間，避免拖慢速度
    nodes_visited++;
    if ((nodes_visited & 2047) == 0) {
        if (!time_up && std::chrono::steady_clock::now() > stop_time) {
            time_up = true; // 標記超時
        }
    }
    // 如果超時，立刻回傳 0 (強制中斷遞迴)
    if (time_up) return 0;

    float original_alpha = alpha;
    float score = 0.0f;
    Move tt_move;

    // 1. Probe Transposition Table
    if (tt_probe(key, depth, alpha, beta, score, tt_move)) {
        return score;
    }

    // 優先處理勝負判斷，給予巨大的分數獎勵
    if (pos.winner() != NO_COLOR) {
        const float WIN_SCORE = 1000000.0f; // 大於任何可能的 evaluate 分數
        
        if (pos.winner() == mySide) {
            // 贏了：分數越高越好。
            // 加上 depth 代表「離根節點越近 (剩餘深度越多)」，即越快獲勝。
            return WIN_SCORE + depth; 
        } else {
            // 輸了：分數越低越好 (負很多)。
            // 減去 depth 代表「越快輸分數越低」，AI 會試圖避免快速輸掉 (選擇 depth 小的路徑)。
            return -WIN_SCORE - depth;
        }
    }

    // 如果沒有分出勝負，且深度耗盡，才使用啟發式評估
    if (depth == 0) {
        return evaluate(pos, mySide);
    }

    MoveList moves(pos);
    if (moves.size() == 0) {
        return evaluate(pos, mySide);
    }

    // Move Ordering:
    // 1. Prioritize the move from the TT
    bool found_tt_move = false;
    if (tt_move != Move()) { // Ensure tt_move is valid
        for (size_t i = 0; i < moves.size(); ++i) {
            if (moves[i] == tt_move) {
                std::swap(moves[0], moves[i]);
                found_tt_move = true;
                break;
            }
        }
    }

    // 2. Sort the rest of the moves using heuristics
    if (moves.size() > 1) {
        size_t sort_start_index = found_tt_move ? 1 : 0;
        std::sort(moves.begin() + sort_start_index, moves.end(),
            [&](const Move& a, const Move& b) {
                return get_move_score(a, pos, mySide) > get_move_score(b, pos, mySide);
            });
    }

    float best_score = -std::numeric_limits<float>::infinity();
    Move best_move_for_node;

    // First move with full window
    {
        Move move = moves[0];
        if (move.type() == Flipping) {
            score = Star1_EQU_F(pos, key, move, depth, alpha, beta, mySide);
        } else {
            Position next_pos = pos;
            uint64_t next_key = key;
            Piece moving_piece = next_pos.peek_piece_at(move.from());
            Piece captured_piece = next_pos.peek_piece_at(move.to());

            update_key(next_key, move.from(), piece_to_index(moving_piece));
            if (captured_piece.type != NO_PIECE) {
                update_key(next_key, move.to(), piece_to_index(captured_piece));
            }
            update_key(next_key, move.to(), piece_to_index(moving_piece));
            
            next_key ^= MY_SIDE_ZOBRIST_KEY;

            next_pos.do_move(move);
            score = G4_NegaScout(next_pos, next_key, depth - 1, alpha, beta, mySide);
        }
        
        if (time_up) return 0; // Check after recursive call

        if (score > best_score) {
            best_score = score;
            best_move_for_node = move;
        }
        alpha = std::max(alpha, best_score);
        
        if (alpha >= beta) {
            tt_store(key, depth, best_score, FLAG_LOWER_BOUND, best_move_for_node);
            return best_score;
        }
    }

    // Subsequent moves with null window
    for (size_t i = 1; i < moves.size(); ++i) {
        Move move = moves[i];
        
        if (move.type() == Flipping) {
            score = Star1_EQU_F(pos, key, move, depth, alpha, alpha + 1, mySide);
        } else {
            Position next_pos = pos;
            uint64_t next_key = key;
            Piece moving_piece = next_pos.peek_piece_at(move.from());
            Piece captured_piece = next_pos.peek_piece_at(move.to());

            update_key(next_key, move.from(), piece_to_index(moving_piece));
            if (captured_piece.type != NO_PIECE) {
                update_key(next_key, move.to(), piece_to_index(captured_piece));
            }
            update_key(next_key, move.to(), piece_to_index(moving_piece));

            next_key ^= MY_SIDE_ZOBRIST_KEY;

            next_pos.do_move(move);
            score = G4_NegaScout(next_pos, next_key, depth - 1, alpha, alpha + 1, mySide);
        }
        
        if (time_up) return 0; // Check after recursive call

        if (score > alpha && score < beta) { // Re-search
            if (move.type() == Flipping) {
                 score = Star1_EQU_F(pos, key, move, depth, alpha, beta, mySide);
            } else {
                 Position research_pos = pos;
                 uint64_t next_key = key;
                 Piece moving_piece = research_pos.peek_piece_at(move.from());
                 Piece captured_piece = research_pos.peek_piece_at(move.to());

                 update_key(next_key, move.from(), piece_to_index(moving_piece));
                 if (captured_piece.type != NO_PIECE) {
                    update_key(next_key, move.to(), piece_to_index(captured_piece));
                 }
                 update_key(next_key, move.to(), piece_to_index(moving_piece));

                 next_key ^= MY_SIDE_ZOBRIST_KEY;
                 
                 research_pos.do_move(move);
                 score = G4_NegaScout(research_pos, next_key, depth - 1, alpha, beta, mySide);
            }
        }
        
        if (time_up) return 0; // Check after recursive call

        if (score > best_score) {
            best_score = score;
            best_move_for_node = move;
        }
        alpha = std::max(alpha, best_score);
        
        if (alpha >= beta) {
            break;
        }
    }

    // 2. Store result in Transposition Table
    TTFlag flag = (best_score > original_alpha) ? FLAG_EXACT : FLAG_UPPER_BOUND;
    if (best_score >= beta) flag = FLAG_LOWER_BOUND;
    tt_store(key, depth, best_score, flag, best_move_for_node);

    return best_score;
}

float G4_NegaScout(Position& pos, uint64_t key, int depth, float alpha, float beta, Color mySide) {
    // 每 2048 個節點檢查一次時間，避免拖慢速度
    nodes_visited++;
    if ((nodes_visited & 2047) == 0) {
        if (!time_up && std::chrono::steady_clock::now() > stop_time) {
            time_up = true; // 標記超時
        }
    }
    // 如果超時，立刻回傳 0 (強制中斷遞迴)
    if (time_up) return 0;

    float original_beta = beta;
    float score = 0.0f;
    Move tt_move;

    // 1. Probe Transposition Table
    if (tt_probe(key, depth, alpha, beta, score, tt_move)) {
        return score;
    }

    // 優先處理勝負判斷，給予巨大的分數獎勵
    if (pos.winner() != NO_COLOR) {
        const float WIN_SCORE = 1000000.0f; // 大於任何可能的 evaluate 分數
        
        if (pos.winner() == mySide) {
            // 贏了：分數越高越好。
            // 加上 depth 代表「離根節點越近 (剩餘深度越多)」，即越快獲勝。
            return WIN_SCORE + depth; 
        } else {
            // 輸了：分數越低越好 (負很多)。
            // 減去 depth 代表「越快輸分數越低」，AI 會試圖避免快速輸掉 (選擇 depth 小的路徑)。
            return -WIN_SCORE - depth;
        }
    }

    // 如果沒有分出勝負，且深度耗盡，才使用啟發式評估
    if (depth == 0) {
        return evaluate(pos, mySide);
    }

    MoveList moves(pos);
    if (moves.size() == 0) {
        return evaluate(pos, mySide);
    }

    // Move Ordering:
    // 1. Prioritize the move from the TT
    bool found_tt_move = false;
    if (tt_move != Move()) { // Ensure tt_move is valid
        for (size_t i = 0; i < moves.size(); ++i) {
            if (moves[i] == tt_move) {
                std::swap(moves[0], moves[i]);
                found_tt_move = true;
                break;
            }
        }
    }

    // 2. Sort the rest of the moves using heuristics
    if (moves.size() > 1) {
        size_t sort_start_index = found_tt_move ? 1 : 0;
        std::sort(moves.begin() + sort_start_index, moves.end(),
            [&](const Move& a, const Move& b) {
                return get_move_score(a, pos, mySide) > get_move_score(b, pos, mySide);
            });
    }

    float best_score = std::numeric_limits<float>::infinity();
    Move best_move_for_node;

    // First move with full window
    {
        Move move = moves[0];
        if (move.type() == Flipping) {
            score = Star1_EQU_G(pos, key, move, depth, alpha, beta, mySide);
        } else {
            Position next_pos = pos;
            uint64_t next_key = key;
            Piece moving_piece = next_pos.peek_piece_at(move.from());
            Piece captured_piece = next_pos.peek_piece_at(move.to());

            update_key(next_key, move.from(), piece_to_index(moving_piece));
            if (captured_piece.type != NO_PIECE) {
                update_key(next_key, move.to(), piece_to_index(captured_piece));
            }
            update_key(next_key, move.to(), piece_to_index(moving_piece));

            next_key ^= MY_SIDE_ZOBRIST_KEY;

            next_pos.do_move(move);
            score = F4_NegaScout(next_pos, next_key, depth - 1, alpha, beta, mySide);
        }
        
        if (time_up) return 0; // Check after recursive call

        if (score < best_score) {
            best_score = score;
            best_move_for_node = move;
        }
        beta = std::min(beta, best_score);

        if (alpha >= beta) {
            tt_store(key, depth, best_score, FLAG_UPPER_BOUND, best_move_for_node);
            return best_score;
        }
    }

    // Subsequent moves with null window
    for (size_t i = 1; i < moves.size(); ++i) {
        Move move = moves[i];

        if (move.type() == Flipping) {
            score = Star1_EQU_G(pos, key, move, depth, beta - 1, beta, mySide);
        } else {
            Position next_pos = pos;
            uint64_t next_key = key;
            Piece moving_piece = next_pos.peek_piece_at(move.from());
            Piece captured_piece = next_pos.peek_piece_at(move.to());

            update_key(next_key, move.from(), piece_to_index(moving_piece));
            if (captured_piece.type != NO_PIECE) {
                update_key(next_key, move.to(), piece_to_index(captured_piece));
            }
            update_key(next_key, move.to(), piece_to_index(moving_piece));

            next_key ^= MY_SIDE_ZOBRIST_KEY;
            
            next_pos.do_move(move);
            score = F4_NegaScout(next_pos, next_key, depth - 1, beta - 1, beta, mySide);
        }
        
        if (time_up) return 0; // Check after recursive call

        if (score < beta && score > alpha) { // Re-search
             if (move.type() == Flipping) {
                 score = Star1_EQU_G(pos, key, move, depth, alpha, beta, mySide);
             } else {
                 Position research_pos = pos;
                 uint64_t next_key = key;
                 Piece moving_piece = research_pos.peek_piece_at(move.from());
                 Piece captured_piece = research_pos.peek_piece_at(move.to());

                 update_key(next_key, move.from(), piece_to_index(moving_piece));
                 if (captured_piece.type != NO_PIECE) {
                    update_key(next_key, move.to(), piece_to_index(captured_piece));
                 }
                 update_key(next_key, move.to(), piece_to_index(moving_piece));

                 next_key ^= MY_SIDE_ZOBRIST_KEY;

                 research_pos.do_move(move);
                 score = F4_NegaScout(research_pos, next_key, depth - 1, alpha, beta, mySide);
             }
        }
        
        if (time_up) return 0; // Check after recursive call

        if (score < best_score) {
            best_score = score;
            best_move_for_node = move;
        }
        beta = std::min(beta, best_score);
        
        if (alpha >= beta) {
            break;
        }
    }

    TTFlag flag = (best_score < original_beta) ? FLAG_EXACT : FLAG_LOWER_BOUND;
    if (best_score <= alpha) flag = FLAG_UPPER_BOUND;
    tt_store(key, depth, best_score, flag, best_move_for_node);

    return best_score;
}