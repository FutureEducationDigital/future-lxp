[@bs.config {jsx: 3}];

open CurriculumEditor__Types;

let str = React.string;

module SortContentBlockMutation = [%graphql
  {|
   mutation($contentBlockIds: [ID!]!) {
    sortContentBlocks(contentBlockIds: $contentBlockIds) {
       success
     }
   }
   |}
];

let updateContentBlockSorting =
    (
      contentBlocks,
      authenticityToken,
      sortContentBlock,
      toggleSortContentBlock,
      (),
    ) => {
  let contentBlockIds =
    contentBlocks
    |> List.map(((_, _, cb, id)) =>
         switch (cb) {
         | Some(_cb) => id
         | None => ""
         }
       )
    |> List.filter(id => id != "")
    |> Array.of_list;

  if (sortContentBlock == true) {
    SortContentBlockMutation.make(~contentBlockIds, ())
    |> GraphqlQuery.sendQuery(authenticityToken, ~notify=false)
    |> Js.Promise.then_(_response => Js.Promise.resolve())
    |> ignore;
    toggleSortContentBlock(_ => false);
  } else {
    ();
  };
  None;
};

let updateContentBlockMasterList =
    (sortedContentBlocks, updateContentBlocksCB, targetId) => {
  let updatedCBs =
    sortedContentBlocks
    |> List.filter(((_, _, cb, _)) =>
         switch (cb) {
         | Some(_) => true
         | None => false
         }
       )
    |> List.map(((sortIndex, blockType, _, id)) =>
         ContentBlock.make(id, blockType, targetId, sortIndex)
       );
  updateContentBlocksCB(targetId, updatedCBs);
};

let removeTargetContentCB =
    (updateTargetContentBlocks, toggleSortContentBlock, sortIndex) => {
  updateTargetContentBlocks(targetContentBlocks =>
    targetContentBlocks
    |> List.filter(((index, _, _, _)) => sortIndex != index)
  );
  toggleSortContentBlock(sortContentBlock => !sortContentBlock);
};

let newContentBlockCB =
    (updateTargetContentBlocks, sortIndex, blockType: ContentBlock.blockType) =>
  updateTargetContentBlocks(targetContentBlocks => {
    let (lowerCBs, upperCBs) =
      targetContentBlocks
      |> List.partition(((index, _, _, _)) => index < sortIndex);
    let newComponentKey = Js.Date.now() |> Js.Float.toString;
    let updatedUpperCBs =
      upperCBs
      |> List.map(((index, blockType, cb, id)) =>
           (index + 1, blockType, cb, id)
         )
      |> List.append([(sortIndex, blockType, None, newComponentKey)]);
    List.append(lowerCBs, updatedUpperCBs);
  });

let swapContentBlockCB =
    (
      targetContentBlocks,
      updateTargetContentBlocks,
      toggleSortContentBlock,
      upperSortIndex,
      lowerSortIndex,
    ) => {
  let upperContentBlock =
    targetContentBlocks
    |> ListUtils.unsafeFind(
         ((index, _, _, _)) => index == upperSortIndex,
         "Unable to find content block with this sort index",
       );
  let lowerContentBlock =
    targetContentBlocks
    |> ListUtils.unsafeFind(
         ((index, _, _, _)) => index == lowerSortIndex,
         "Unable to find content block with this sort index",
       );
  let (sortIndex1, blockType1, cb1, id1) = lowerContentBlock;
  let (sortIndex2, blockType2, cb2, id2) = upperContentBlock;

  updateTargetContentBlocks(targetContentBlocks =>
    targetContentBlocks
    |> List.filter(((index, _, _, _)) =>
         !([lowerSortIndex, upperSortIndex] |> List.mem(index))
       )
    |> List.append([
         (sortIndex1, blockType2, cb2, id2),
         (sortIndex2, blockType1, cb1, id1),
       ])
  );
  toggleSortContentBlock(sortContentBlock => !sortContentBlock);
};

let createNewContentCB = (updateTargetContentBlocks, contentBlock) => {
  let newContentBlock = (
    ContentBlock.sortIndex(contentBlock),
    ContentBlock.blockType(contentBlock),
    Some(contentBlock),
    ContentBlock.id(contentBlock),
  );
  updateTargetContentBlocks(targetContentBlocks =>
    targetContentBlocks
    |> List.filter(((index, _, _, _)) =>
         index != ContentBlock.sortIndex(contentBlock)
       )
    |> List.append([newContentBlock])
  );
};

let updateContentBlockCB = (updateTargetContentBlocks, contentBlock) => {
  let newContentBlock = (
    ContentBlock.sortIndex(contentBlock),
    ContentBlock.blockType(contentBlock),
    Some(contentBlock),
    ContentBlock.id(contentBlock),
  );

  updateTargetContentBlocks(targetContentBlocks =>
    targetContentBlocks
    |> List.filter(((_, _, _, id)) => id != ContentBlock.id(contentBlock))
    |> List.append([newContentBlock])
  );
};

[@react.component]
let make =
    (
      ~target,
      ~contentBlocks,
      ~updateContentBlocksCB,
      ~updateContentEditorDirtyCB,
      ~authenticityToken,
    ) => {
  let (targetContentBlocks, updateTargetContentBlocks) =
    React.useState(() =>
      contentBlocks
      |> List.sort((x, y) =>
           ContentBlock.sortIndex(x) - ContentBlock.sortIndex(y)
         )
      |> List.mapi((index, cb) =>
           (
             index + 1,
             cb |> ContentBlock.blockType,
             Some(cb),
             cb |> ContentBlock.id,
           )
         )
    );
  let (sortContentBlock, toggleSortContentBlock) =
    React.useState(() => false);

  let sortedContentBlocks =
    targetContentBlocks
    |> List.sort((x, y) => {
         let (x, _, _, _) = x;
         let (y, _, _, _) = y;
         x - y;
       });

  React.useEffect1(
    updateContentBlockSorting(
      sortedContentBlocks,
      authenticityToken,
      sortContentBlock,
      toggleSortContentBlock,
    ),
    [|sortContentBlock|],
  );

  let contentEditorDirty =
    targetContentBlocks |> List.exists(((_, _, cb, _)) => cb == None);

  React.useEffect1(
    () => {
      updateContentEditorDirtyCB(contentEditorDirty);

      updateContentBlockMasterList(
        targetContentBlocks,
        updateContentBlocksCB,
        target |> Target.id,
      );
      None;
    },
    [|contentEditorDirty|],
  );

  [|
    <CurriculumEditor__ContentTypePicker
      key="static-content-picker"
      sortIndex={
        switch (sortedContentBlocks) {
        | [] => 1
        | nonEmptyList =>
          let (sortIndex, _, _, _) = nonEmptyList |> List.rev |> List.hd;
          sortIndex + 1;
        }
      }
      staticMode=true
      newContentBlockCB={newContentBlockCB(updateTargetContentBlocks)}
    />,
  |]
  |> Array.append(
       sortedContentBlocks
       |> List.map(((sortIndex, blockType, contentBlock, id)) =>
            <CurriculumEditor__ContentBlockEditor
              key=id
              editorId=id
              target
              contentBlock
              removeTargetContentCB={
                removeTargetContentCB(
                  updateTargetContentBlocks,
                  toggleSortContentBlock,
                )
              }
              blockType
              sortIndex
              newContentBlockCB={newContentBlockCB(updateTargetContentBlocks)}
              createNewContentCB={
                createNewContentCB(updateTargetContentBlocks)
              }
              updateContentBlockCB={
                updateContentBlockCB(updateTargetContentBlocks)
              }
              blockCount={targetContentBlocks |> List.length}
              swapContentBlockCB={
                swapContentBlockCB(
                  targetContentBlocks,
                  updateTargetContentBlocks,
                  toggleSortContentBlock,
                )
              }
              targetContentBlocks
              authenticityToken
            />
          )
       |> Array.of_list,
     )
  |> React.array;
};