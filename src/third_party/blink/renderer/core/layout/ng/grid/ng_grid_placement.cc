// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file

#include "third_party/blink/renderer/core/layout/ng/grid/ng_grid_placement.h"

namespace blink {

NGGridPlacement::NGGridPlacement(const ComputedStyle& grid_style,
                                 const wtf_size_t column_auto_repetitions,
                                 const wtf_size_t row_auto_repetitions,
                                 const wtf_size_t column_start_offset,
                                 const wtf_size_t row_start_offset)
    : grid_style_(grid_style),
      packing_behavior_(grid_style.IsGridAutoFlowAlgorithmSparse()
                            ? PackingBehavior::kSparse
                            : PackingBehavior::kDense),
      // The major direction is the one specified in the 'grid-auto-flow'
      // property (row or column), the minor direction is its opposite.
      major_direction_(grid_style.IsGridAutoFlowDirectionRow() ? kForRows
                                                               : kForColumns),
      minor_direction_(grid_style.IsGridAutoFlowDirectionRow() ? kForColumns
                                                               : kForRows),
      column_auto_repeat_track_count_(
          grid_style.GridTemplateColumns().NGTrackList().AutoRepeatSize() *
          column_auto_repetitions),
      row_auto_repeat_track_count_(
          grid_style.GridTemplateRows().NGTrackList().AutoRepeatSize() *
          row_auto_repetitions),
      column_auto_repetitions_(column_auto_repetitions),
      row_auto_repetitions_(row_auto_repetitions),
      column_start_offset_(column_start_offset),
      row_start_offset_(row_start_offset) {}

// https://drafts.csswg.org/css-grid/#auto-placement-algo
void NGGridPlacement::RunAutoPlacementAlgorithm(GridItems* grid_items) {
  DCHECK(grid_items);

  // Step 1. Position anything that’s not auto-placed; if no items need
  // auto-placement, then we are done.
  PlacedGridItemsList placed_items;
  GridItemVector items_locked_to_major_axis;
  GridItemVector items_not_locked_to_major_axis;

  if (!PlaceNonAutoGridItems(grid_items, &items_locked_to_major_axis,
                             &items_not_locked_to_major_axis, &placed_items)) {
    return;
  }
  placed_items.AppendCurrentItemsToOrderedList();

  // Step 2. Process the items locked to the major axis.
  PlaceGridItemsLockedToMajorAxis(items_locked_to_major_axis, &placed_items);

  // Step 3. Determine the number of minor tracks in the implicit grid.
  // This is already accomplished within the |PlaceNonAutoGridItems| and
  // |PlaceGridItemsLockedToMajorAxis| methods; nothing else to do here.

  // Step 4. Position remaining grid items.
  AutoPlacementCursor placement_cursor(placed_items.FirstPlacedItem());
  for (GridItemData* grid_item : items_not_locked_to_major_axis) {
    switch (grid_item->AutoPlacement(major_direction_)) {
      case AutoPlacementType::kBoth:
        PlaceAutoBothAxisGridItem(grid_item, &placed_items, &placement_cursor);
        break;
      case AutoPlacementType::kMajor:
        PlaceAutoMajorAxisGridItem(grid_item, &placed_items, &placement_cursor);
        break;
      case AutoPlacementType::kMinor:
      case AutoPlacementType::kNotNeeded:
        NOTREACHED() << "Placement of non-auto placed items and items locked "
                        "to a major axis should've already occurred.";
        break;
    }
    if (!HasSparsePacking()) {
      // For dense packing, set the cursor’s major and minor positions to the
      // start-most row and column lines in the implicit grid.
      placement_cursor = AutoPlacementCursor(placed_items.FirstPlacedItem());
    }
  }
}

bool NGGridPlacement::PlaceNonAutoGridItems(
    GridItems* grid_items,
    GridItemVector* items_locked_to_major_axis,
    GridItemVector* items_not_locked_to_major_axis,
    PlacedGridItemsList* placed_items) {
  DCHECK(grid_items && items_locked_to_major_axis &&
         items_not_locked_to_major_axis && placed_items);

  column_start_offset_ = row_start_offset_ = 0;
  for (auto& grid_item : grid_items->item_data) {
    const auto& item_style = grid_item.node.Style();

    GridSpan item_column_span =
        GridPositionsResolver::ResolveGridPositionsFromStyle(
            grid_style_, item_style, kForColumns,
            column_auto_repeat_track_count_);
    DCHECK(!item_column_span.IsTranslatedDefinite());
    grid_item.SetSpan(item_column_span, kForColumns);

    GridSpan item_row_span =
        GridPositionsResolver::ResolveGridPositionsFromStyle(
            grid_style_, item_style, kForRows, row_auto_repeat_track_count_);
    DCHECK(!item_row_span.IsTranslatedDefinite());
    grid_item.SetSpan(item_row_span, kForRows);

    // When we have negative indices that go beyond the start of the explicit
    // grid we need to prepend tracks to it; count how many tracks are needed by
    // checking the minimum negative start line of definite spans, the negative
    // of that minimum is the number of tracks we need to prepend.
    // Simplifying the logic above: maximize the negative value of start lines.
    if (item_column_span.IsUntranslatedDefinite()) {
      column_start_offset_ = std::max<int>(
          column_start_offset_, -item_column_span.UntranslatedStartLine());
    }
    if (item_row_span.IsUntranslatedDefinite()) {
      row_start_offset_ = std::max<int>(row_start_offset_,
                                        -item_row_span.UntranslatedStartLine());
    }
  }

  minor_max_end_line_ =
      (minor_direction_ == kForColumns)
          ? GridPositionsResolver::ExplicitGridColumnCount(
                grid_style_, column_auto_repeat_track_count_) +
                column_start_offset_
          : GridPositionsResolver::ExplicitGridRowCount(
                grid_style_, row_auto_repeat_track_count_) +
                row_start_offset_;

  placed_items->needs_to_sort_item_vector = false;
  auto& non_auto_placed_items = placed_items->item_vector;
  non_auto_placed_items.ReserveInitialCapacity(grid_items->Size());

  for (auto& grid_item : grid_items->item_data) {
    GridSpan item_major_span = grid_item.Span(major_direction_);
    GridSpan item_minor_span = grid_item.Span(minor_direction_);

    const bool has_indefinite_major_span = item_major_span.IsIndefinite();
    const bool has_indefinite_minor_span = item_minor_span.IsIndefinite();

    if (!has_indefinite_major_span) {
      item_major_span.Translate(StartOffset(major_direction_));
      grid_item.SetSpan(item_major_span, major_direction_);
    }
    if (!has_indefinite_minor_span) {
      item_minor_span.Translate(StartOffset(minor_direction_));
      grid_item.SetSpan(item_minor_span, minor_direction_);
    }

    minor_max_end_line_ = std::max<wtf_size_t>(
        minor_max_end_line_, has_indefinite_minor_span
                                 ? item_minor_span.IndefiniteSpanSize()
                                 : item_minor_span.EndLine());

    if (!has_indefinite_major_span && !has_indefinite_minor_span) {
      auto* placed_item =
          new PlacedGridItem(grid_item, major_direction_, minor_direction_);

      // We will need to sort the item vector if the new placed item should be
      // inserted to the ordered list before the last item in the vector.
      placed_items->needs_to_sort_item_vector |=
          !non_auto_placed_items.IsEmpty() &&
          *placed_item < *non_auto_placed_items.back();

      non_auto_placed_items.emplace_back(placed_item);
    } else {
      if (has_indefinite_major_span)
        items_not_locked_to_major_axis->emplace_back(&grid_item);
      else
        items_locked_to_major_axis->emplace_back(&grid_item);
    }
  }
  return !items_not_locked_to_major_axis->IsEmpty() ||
         !items_locked_to_major_axis->IsEmpty();
}

void NGGridPlacement::PlaceGridItemsLockedToMajorAxis(
    const GridItemVector& items_locked_to_major_axis,
    PlacedGridItemsList* placed_items) {
  DCHECK(placed_items);

  // Mapping between the major axis tracks and the last auto-placed item's end
  // line inserted on that track. This is needed to implement "sparse" packing
  // for grid items locked to a given major axis track.
  // See https://drafts.csswg.org/css-grid/#auto-placement-algo.
  HashMap<wtf_size_t, wtf_size_t, DefaultHash<wtf_size_t>::Hash,
          WTF::UnsignedWithZeroKeyHashTraits<wtf_size_t>>
      minor_cursors;

  for (GridItemData* grid_item : items_locked_to_major_axis) {
    DCHECK_EQ(grid_item->AutoPlacement(major_direction_),
              AutoPlacementType::kMinor);

    const wtf_size_t minor_span_size =
        grid_item->Span(minor_direction_).IndefiniteSpanSize();
    const wtf_size_t major_start_line = grid_item->StartLine(major_direction_);

    AutoPlacementCursor placement_cursor(placed_items->FirstPlacedItem());
    placement_cursor.MoveToMajorLine(major_start_line);
    if (HasSparsePacking() && minor_cursors.Contains(major_start_line))
      placement_cursor.MoveToMinorLine(minor_cursors.at(major_start_line));

    placement_cursor.MoveCursorToFitGridSpan(
        grid_item->SpanSize(major_direction_), minor_span_size,
        minor_max_end_line_, CursorMovementBehavior::kForceMajorLine);

    wtf_size_t minor_end_line = placement_cursor.MinorLine() + minor_span_size;
    if (HasSparsePacking())
      minor_cursors.Set(major_start_line, minor_end_line);
    minor_max_end_line_ = std::max(minor_max_end_line_, minor_end_line);

    // Update grid item placement for minor axis.
    GridSpan grid_item_span = GridSpan::TranslatedDefiniteGridSpan(
        placement_cursor.MinorLine(), minor_end_line);
    grid_item->SetSpan(grid_item_span, minor_direction_);

    PlaceGridItemAtCursor(*grid_item, placed_items, &placement_cursor);
  }
}

void NGGridPlacement::PlaceAutoMajorAxisGridItem(
    GridItemData* grid_item,
    PlacedGridItemsList* placed_items,
    AutoPlacementCursor* placement_cursor) const {
  DCHECK(grid_item && placed_items);
  const wtf_size_t major_span_size =
      grid_item->Span(major_direction_).IndefiniteSpanSize();

  placement_cursor->MoveToMinorLine(grid_item->StartLine(minor_direction_));
  placement_cursor->MoveCursorToFitGridSpan(
      major_span_size, grid_item->SpanSize(minor_direction_),
      minor_max_end_line_, CursorMovementBehavior::kForceMinorLine);

  // Update grid item placement for major axis.
  GridSpan grid_item_span = GridSpan::TranslatedDefiniteGridSpan(
      placement_cursor->MajorLine(),
      placement_cursor->MajorLine() + major_span_size);
  grid_item->SetSpan(grid_item_span, major_direction_);

  PlaceGridItemAtCursor(*grid_item, placed_items, placement_cursor);
}

void NGGridPlacement::PlaceAutoBothAxisGridItem(
    GridItemData* grid_item,
    PlacedGridItemsList* placed_items,
    AutoPlacementCursor* placement_cursor) const {
  DCHECK(grid_item && placed_items && placement_cursor);

  const wtf_size_t major_span_size =
      grid_item->Span(major_direction_).IndefiniteSpanSize();
  const wtf_size_t minor_span_size =
      grid_item->Span(minor_direction_).IndefiniteSpanSize();

  placement_cursor->MoveCursorToFitGridSpan(major_span_size, minor_span_size,
                                            minor_max_end_line_,
                                            CursorMovementBehavior::kAuto);

  // Update grid item placement for both axis.
  GridSpan grid_item_span = GridSpan::TranslatedDefiniteGridSpan(
      placement_cursor->MajorLine(),
      placement_cursor->MajorLine() + major_span_size);
  grid_item->SetSpan(grid_item_span, major_direction_);

  grid_item_span = GridSpan::TranslatedDefiniteGridSpan(
      placement_cursor->MinorLine(),
      placement_cursor->MinorLine() + minor_span_size);
  grid_item->SetSpan(grid_item_span, minor_direction_);

  PlaceGridItemAtCursor(*grid_item, placed_items, placement_cursor);
}

void NGGridPlacement::PlaceGridItemAtCursor(
    const GridItemData& grid_item,
    PlacedGridItemsList* placed_items,
    AutoPlacementCursor* placement_cursor) const {
  DCHECK(placed_items && placement_cursor);

  auto* new_placed_item =
      new PlacedGridItem(grid_item, major_direction_, minor_direction_);
  placed_items->item_vector.emplace_back(new_placed_item);

  const auto* next_placed_item = placement_cursor->NextPlacedItem();
  placed_items->ordered_list.InsertAfter(
      new_placed_item, next_placed_item ? next_placed_item->Prev()
                                        : placed_items->ordered_list.Tail());

  placement_cursor->InsertPlacedItemAtCurrentPosition(new_placed_item);
}

wtf_size_t NGGridPlacement::AutoRepeatTrackCount(
    const GridTrackSizingDirection track_direction) const {
  return (track_direction == kForColumns) ? column_auto_repeat_track_count_
                                          : row_auto_repeat_track_count_;
}

wtf_size_t NGGridPlacement::AutoRepetitions(
    const GridTrackSizingDirection track_direction) const {
  return (track_direction == kForColumns) ? column_auto_repetitions_
                                          : row_auto_repetitions_;
}

wtf_size_t NGGridPlacement::StartOffset(
    const GridTrackSizingDirection track_direction) const {
  return (track_direction == kForColumns) ? column_start_offset_
                                          : row_start_offset_;
}

bool NGGridPlacement::HasSparsePacking() const {
  return packing_behavior_ == PackingBehavior::kSparse;
}

// A grid position is defined as the intersection between a line from the major
// axis and another from the minor axis. Following the auto-placement algorithm
// convention, a position with lesser major axis line comes first; in case of
// ties, a position with lesser minor axis line comes first.
bool NGGridPlacement::GridPosition::operator<=(
    const GridPosition& other) const {
  return (major_line == other.major_line) ? minor_line <= other.minor_line
                                          : major_line < other.major_line;
}
bool NGGridPlacement::GridPosition::operator<(const GridPosition& other) const {
  return (major_line != other.major_line) ? major_line < other.major_line
                                          : minor_line < other.minor_line;
}

NGGridPlacement::PlacedGridItem::PlacedGridItem(
    const GridItemData& grid_item,
    const GridTrackSizingDirection major_direction,
    const GridTrackSizingDirection minor_direction)
    : start_{grid_item.StartLine(major_direction),
             grid_item.StartLine(minor_direction)},
      end_{grid_item.EndLine(major_direction),
           grid_item.EndLine(minor_direction)} {}

NGGridPlacement::GridPosition
NGGridPlacement::PlacedGridItem::EndOnPreviousMajorLine() const {
  DCHECK_GT(end_.major_line, 0u);
  return {end_.major_line - 1, end_.minor_line};
}

void NGGridPlacement::AutoPlacementCursor::MoveCursorToFitGridSpan(
    const wtf_size_t major_span_size,
    const wtf_size_t minor_span_size,
    const wtf_size_t minor_max_end_line,
    const CursorMovementBehavior movement_behavior) {
  DCHECK_LE(minor_span_size, minor_max_end_line);

  wtf_size_t next_minor_line;
  const bool allow_minor_line_movement =
      movement_behavior != CursorMovementBehavior::kForceMinorLine;

  // If we want to force the current major line, it's okay to place this grid
  // span beyond the implicit grid's maximum minor end line.
  const wtf_size_t minor_max_start_line =
      (movement_behavior == CursorMovementBehavior::kForceMajorLine)
          ? minor_max_end_line
          : minor_max_end_line - minor_span_size;

  auto NeedsToMoveToNextMajorLine = [&]() -> bool {
    // If we need to force the minor line, or the grid span would go beyond the
    // maximum minor end line, there is no point to keep looking for overlapping
    // items in the current major line, i.e. needs to move the major line.
    return next_minor_line > minor_max_start_line ||
           (!allow_minor_line_movement &&
            next_minor_line != current_position_.minor_line);
  };

  auto DoesCurrentPositionFitGridSpan = [&]() -> bool {
    if (NeedsToMoveToNextMajorLine()) {
      MoveToNextMajorLine(allow_minor_line_movement);
    } else {
      // If the minor line didn't move, it means there was no overlap with any
      // previously placed item, and we don't need to move any further.
      if (current_position_.minor_line == next_minor_line)
        return true;

      DCHECK_LT(current_position_.minor_line, next_minor_line);
      MoveToMinorLine(next_minor_line);
    }
    return false;
  };

  if (current_position_.minor_line > minor_max_start_line)
    MoveToNextMajorLine(allow_minor_line_movement);

  while (true) {
    UpdateItemsOverlappingMajorLine();
    next_minor_line = current_position_.minor_line;
    for (const auto* placed_item : items_overlapping_major_line_) {
      const wtf_size_t minor_span_end_line = next_minor_line + minor_span_size;
      const wtf_size_t item_minor_end_line = placed_item->MinorEndLine();

      // Since we know that this item will overlap with the current major line,
      // we only need to check if the minor span will overlap too.
      if (next_minor_line < item_minor_end_line &&
          placed_item->MinorStartLine() < minor_span_end_line) {
        next_minor_line = item_minor_end_line;
        if (NeedsToMoveToNextMajorLine())
          break;
      }
    }

    // If the next minor line was moved because it overlapped with a placed
    // item, we don't need to check for overlaps with the rest of the upcoming
    // placed items; keep looking for a position that doesn't overlap with the
    // set of items overlapping the current major line first.
    if (!DoesCurrentPositionFitGridSpan())
      continue;

    const auto* upcoming_item = next_placed_item_;
    while (upcoming_item) {
      const wtf_size_t major_span_end_line =
          current_position_.major_line + major_span_size;
      const wtf_size_t minor_span_end_line = next_minor_line + minor_span_size;
      const wtf_size_t item_minor_end_line = upcoming_item->MinorEndLine();

      // Check if the cursor would overlap the upcoming placed item.
      if (next_minor_line < item_minor_end_line &&
          current_position_.major_line < upcoming_item->MajorEndLine() &&
          upcoming_item->MajorStartLine() < major_span_end_line &&
          upcoming_item->MinorStartLine() < minor_span_end_line) {
        next_minor_line = item_minor_end_line;
        if (NeedsToMoveToNextMajorLine())
          break;
      }
      upcoming_item = upcoming_item->Next();
    }

    if (DoesCurrentPositionFitGridSpan()) {
      // No overlap with any placed item.
      break;
    }
  }
}

void NGGridPlacement::AutoPlacementCursor::UpdateItemsOverlappingMajorLine() {
  DCHECK(std::is_heap(items_overlapping_major_line_.begin(),
                      items_overlapping_major_line_.end(),
                      ComparePlacedGridItemsByEnd));

  // Notice that the |EndOnPreviousMajorLine| of an item "A" is the first
  // position such that any upcoming grid position (located at a greater
  // major/minor position) is guaranteed to not overlap with "A".
  while (!items_overlapping_major_line_.IsEmpty() &&
         items_overlapping_major_line_.front()->EndOnPreviousMajorLine() <=
             current_position_) {
    std::pop_heap(items_overlapping_major_line_.begin(),
                  items_overlapping_major_line_.end(),
                  ComparePlacedGridItemsByEnd);
    items_overlapping_major_line_.pop_back();
  }

  while (next_placed_item_ && next_placed_item_->Start() <= current_position_) {
    if (current_position_ < next_placed_item_->EndOnPreviousMajorLine()) {
      items_overlapping_major_line_.emplace_back(next_placed_item_);
      std::push_heap(items_overlapping_major_line_.begin(),
                     items_overlapping_major_line_.end(),
                     ComparePlacedGridItemsByEnd);
      has_new_item_overlapping_major_line_ = true;
    }
    next_placed_item_ = next_placed_item_->Next();
  }
}

void NGGridPlacement::AutoPlacementCursor::MoveToMajorLine(
    const wtf_size_t major_line) {
  DCHECK_LE(current_position_.major_line, major_line);
  current_position_.major_line = major_line;
}

void NGGridPlacement::AutoPlacementCursor::MoveToMinorLine(
    const wtf_size_t minor_line) {
  // Since the auto-placement cursor only moves forward to the next minor line,
  // if the cursor is located at a position after the minor line we want to
  // force, cycle back to such minor line in the next major line.
  if (minor_line < current_position_.minor_line)
    ++current_position_.major_line;
  current_position_.minor_line = minor_line;
}

void NGGridPlacement::AutoPlacementCursor::MoveToNextMajorLine(
    const bool allow_minor_line_movement) {
  ++current_position_.major_line;

  if (!has_new_item_overlapping_major_line_ &&
      !items_overlapping_major_line_.IsEmpty()) {
    DCHECK_GE(items_overlapping_major_line_.front()->MajorEndLine(),
              current_position_.major_line);
    current_position_.major_line =
        items_overlapping_major_line_.front()->MajorEndLine();
  }

  if (allow_minor_line_movement)
    current_position_.minor_line = 0;
  has_new_item_overlapping_major_line_ = false;
}

void NGGridPlacement::AutoPlacementCursor::InsertPlacedItemAtCurrentPosition(
    const PlacedGridItem* new_placed_item) {
  // This update must happen after the doubly linked list already updated its
  // element links to keep the necessary order for the cursor's logic.
#if DCHECK_IS_ON()
  if (next_placed_item_) {
    DCHECK_EQ(next_placed_item_->Prev(), new_placed_item);
    DCHECK(*new_placed_item < *next_placed_item_);
  }
#endif
  DCHECK_EQ(new_placed_item->Next(), next_placed_item_);
  next_placed_item_ = new_placed_item;

  MoveToMinorLine(new_placed_item->MinorEndLine());
  has_new_item_overlapping_major_line_ = true;
}

void NGGridPlacement::PlacedGridItemsList::AppendCurrentItemsToOrderedList() {
  DCHECK(ordered_list.IsEmpty());

  auto ComparePlacedGridItemPointers =
      [](const std::unique_ptr<PlacedGridItem>& lhs,
         const std::unique_ptr<PlacedGridItem>& rhs) { return *lhs < *rhs; };

  if (needs_to_sort_item_vector) {
    std::sort(item_vector.begin(), item_vector.end(),
              ComparePlacedGridItemPointers);
  }
  DCHECK(std::is_sorted(item_vector.begin(), item_vector.end(),
                        ComparePlacedGridItemPointers));

  for (auto& placed_item : item_vector)
    ordered_list.Append(placed_item.get());
}

namespace {

bool IsStartLineAuto(const GridTrackSizingDirection track_direction,
                     const ComputedStyle& out_of_flow_item_style) {
  return (track_direction == kForColumns)
             ? out_of_flow_item_style.GridColumnStart().IsAuto()
             : out_of_flow_item_style.GridRowStart().IsAuto();
}

bool IsEndLineAuto(const GridTrackSizingDirection track_direction,
                   const ComputedStyle& out_of_flow_item_style) {
  return (track_direction == kForColumns)
             ? out_of_flow_item_style.GridColumnEnd().IsAuto()
             : out_of_flow_item_style.GridRowEnd().IsAuto();
}

}  // namespace

void NGGridPlacement::ResolveOutOfFlowItemGridLines(
    const NGGridLayoutAlgorithmTrackCollection& track_collection,
    const ComputedStyle& out_of_flow_item_style,
    wtf_size_t* start_line,
    wtf_size_t* end_line) const {
  DCHECK(start_line);
  DCHECK(end_line);

  const GridTrackSizingDirection track_direction = track_collection.Direction();
  GridSpan span = GridPositionsResolver::ResolveGridPositionsFromStyle(
      grid_style_, out_of_flow_item_style, track_direction,
      AutoRepeatTrackCount(track_direction));

  if (span.IsIndefinite()) {
    *start_line = kNotFound;
    *end_line = kNotFound;
    return;
  }

  wtf_size_t start_offset = StartOffset(track_direction);
  int span_start_line = span.UntranslatedStartLine() + start_offset;
  int span_end_line = span.UntranslatedEndLine() + start_offset;

  if (span_start_line < 0 ||
      IsStartLineAuto(track_direction, out_of_flow_item_style) ||
      !track_collection.IsGridLineWithinImplicitGrid(span_start_line)) {
    *start_line = kNotFound;
  } else {
    *start_line = span_start_line;
  }
  if (span_end_line < 0 ||
      IsEndLineAuto(track_direction, out_of_flow_item_style) ||
      !track_collection.IsGridLineWithinImplicitGrid(span_end_line)) {
    *end_line = kNotFound;
  } else {
    *end_line = span_end_line;
  }
}

}  // namespace blink
